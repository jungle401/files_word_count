#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <algorithm>
#include <filesystem>

#include <config.h>

using namespace std;
namespace fs = std::filesystem;
size_t shm_size;

struct RangeReadFiles {
  int start_file;
  streampos start_pos;
  int end_file;
  streampos end_pos;
};

void child_process(
  int child_id,
  int shmid,
  const vector<fs::path>& files,
  const vector<RangeReadFiles>& ranges_read_file
) {
  // Timing start
  auto start = std::chrono::steady_clock::now();

  auto word_count = unordered_map<string, int>();
  auto& rng = ranges_read_file[child_id];
  for (int i = rng.start_file; i <= rng.end_file; i++) {
    auto fin = ifstream(files[i]);
    if (i == rng.start_file) {
      fin.seekg(rng.start_pos);
    }
    auto str = string();
    if (i < rng.end_file) {
      while (fin >> str) {
        word_count[str]++;
      }
    } else {
      while (fin >> str && fin.tellg() < rng.end_pos) {
        word_count[str]++;
      }
    }
  }
  // Serialiation
  // write into the shared memory
  // Attach, to get the pointer to the address of the shared memory.
  char* shared_memory = (char*)shmat(shmid, NULL, 0);
  if (shared_memory == (char*)(-1)) {
    perror("shmat");
    exit(EXIT_FAILURE);
  }

  // format: [string_size string occ]
  char* ptr = shared_memory;
  for (auto& [word, count] : word_count) {
    // see if the next word exceeds the effective memory
    if (ptr + sizeof(word) + sizeof(int) * 2 >= shared_memory + shm_size) {
      perror("shm space limit exceeds\n");
      exit(-1);
    }
    int word_len = word.size();
    *((int*)ptr) = word_len;
    ptr += sizeof(int);
    memcpy(ptr, word.c_str(), word_len);
    ptr += word_len;
    *((int*)ptr) = count;
    ptr += sizeof(int);
  }
  *ptr = '\0';
  shmdt(shared_memory);

  // Timing End
  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  cout << "Child process " << getpid() << " completed in " << duration << " milliseconds" << endl;
}

std::streampos find_next_space(const fs::path& fname, std::streampos start) {
  auto file = ifstream(fname);
  file.seekg(start);
  char ch;
  while (file.get(ch)) {
    if (ch == ' ') {
      return file.tellg();
    }
  }
  return file.tellg(); // Return end of file if no space is found
}

vector<RangeReadFiles> get_ranges_read_file(const vector<fs::path>& files, int num_children) {
  auto res = vector<RangeReadFiles>();
  auto total_size = 0;
  for (const auto& f : files) {
    total_size += fs::file_size(f);
  }
  auto seg_size = total_size / num_children;
  shm_size = seg_size * 4;
  auto acc_size = 0;
  // Find cut points
  auto f = 0;
  for (int i = 1; i < num_children; i++) {
    while (acc_size + fs::file_size(files[f]) < seg_size * i) {
      acc_size += fs::file_size(files[f]);
      f++;
    }
    auto end_stream_pos = find_next_space(files[f], seg_size * i - acc_size);
    res.push_back(RangeReadFiles{0, 0, f, end_stream_pos});
  }
  res.push_back(RangeReadFiles{0, 0, int(files.size() - 1), fs::file_size(files[files.size() - 1])});
  for (int i = 1; i < res.size(); i++) {
    res[i].start_file = res[i - 1].end_file;
    res[i].start_pos = res[i - 1].end_pos;
  }
  return res;
}

int main(int argc, char** argv) {

  // Number of sub-processes
  auto num_children = stoi(argv[argc - 1]);

  // Input files
  auto files = vector<fs::path>();
  for (int i = 1; i < argc - 1; i++) {
    files.push_back(fs::path(argv[i]));
  }
  // auto data_path = fs::path(REPO_PATH) / "directory_big";
  // for (auto& dir_entry : fs::recursive_directory_iterator(data_path)) {
  //   if (fs::is_regular_file(dir_entry)) {
  //     files.push_back(dir_entry);
  //   }
  // }

  // Divide raed file ranges for child processes to read evenly.
  auto ranges_read_file = get_ranges_read_file(files, num_children);

  // Get the vector of id of shared memory for each sub-processes
  auto shmids = vector<int>(num_children);
  for (int i = 0; i < num_children; i++) {
    shmids[i] = shmget(i, shm_size, 0666 | IPC_CREAT);
    if (shmids[i] == -1) {
      perror("shmget");
      exit(EXIT_FAILURE);
    }
  }

  // Fork sub-processes
  for (int i = 0; i < num_children; i++) {
    pid_t pid = fork();
    if (pid == 0) {
      // child process
      child_process(i, shmids[i], files, ranges_read_file);
      exit(0);
    } else if (pid < 0) {
      perror("fork()");
      abort();
    }
  }

  // Join sub-processes
  for (int i = 0; i < num_children; i++) {
    wait(NULL);
  }

  // Deserialize from shared memory and merge into the final word count map.
  auto word_count = unordered_map<string, int>();
  for (int i = 0; i < num_children; i++)  {
    char* shared_memory = (char*)shmat(shmids[i], NULL, 0);
    char* ptr = shared_memory;
    while (ptr < shared_memory + shm_size && *ptr != '\0') {
      auto word_len = *((int*)ptr);
      ptr += sizeof(int);
      auto str = string(ptr, word_len);
      ptr += word_len;
      auto count = *((int*)ptr);
      ptr += sizeof(int);
      word_count[str] += count;
    }
  }

  // Cleanup shared memory
  for (int i = 0; i < num_children; i++)  {
    shmctl(shmids[i], IPC_RMID, NULL);
  }

  // Output response
  for (auto& [word, count] : word_count) {
    cout << word << '\t';
    cout << count << '\n';
  }
  exit(0);

  // Sort the results and output to file. This is for self-defined correctness verification.
  auto output_file = string(REPO_PATH) + "/output/word_counts.txt";
  auto v = vector<pair<string, int>>();
  for (auto& [word, count] : word_count) {
    v.push_back({word, count});
  }
  sort(v.begin(), v.end());
  auto fout = ofstream(output_file);
  for (auto& [word, count] : v) {
    fout << word << '\t';
    fout << count << '\n';
  }
}

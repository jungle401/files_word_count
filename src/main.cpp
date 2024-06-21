#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
// #include <semaphore.h>
#include <fcntl.h>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <algorithm>

#define SHM_SIZE 65536

using namespace std;

vector<string> files =
  {
    "../essay/e1.txt",
    "../essay/e2.txt",
    "../essay/e3.txt",
  };

void child_process(int child_id, int shmid) {
  auto word_count = unordered_map<string, int>();
  auto fin = ifstream(files[child_id]);
  auto str = string();
  while (fin >> str) {
    word_count[str]++;
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
    if (ptr + sizeof(word) + sizeof(int) * 2 >= shared_memory + SHM_SIZE) {
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
}

int main() {

  // Child Processes
  auto num_children = 3;

  // Shared Memory for each sub-processes
  auto shmids = vector<int>(num_children);
  for (int i = 0; i < num_children; i++) {
    shmids[i] = shmget(i, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmids[i] == -1) {
      perror("shmget");
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < num_children; i++) {
    pid_t pid = fork();
    if (pid == 0) {
      // child process
      child_process(i, shmids[i]);
      exit(0);
    } else if (pid < 0) {
      perror("fork()");
      abort();
    }
  }

  for (int i = 0; i < num_children; i++) {
    wait(NULL);
  }

  // Deserialize
  // again attach the pointner staring positioni of the shared memory,
  // parse the content until the null terminated symbol.
  auto word_count = unordered_map<string, int>();
  for (int i = 0; i < num_children; i++)  {
    char* shared_memory = (char*)shmat(shmids[i], NULL, 0);
    // The first sizeof(int) bytes are used for keeping write offset for children
    // processes to communicate.
    char* ptr = shared_memory;
    while (ptr < shared_memory + SHM_SIZE && *ptr != '\0') {
      auto word_len = *((int*)ptr);
      ptr += sizeof(int);
      auto str = string(ptr, word_len);
      ptr += word_len;
      auto count = *((int*)ptr);
      ptr += sizeof(int);
      word_count[str] += count;
    }
  }

  // Cleanup
  // - shared memory
  for (int i = 0; i < num_children; i++)  {
    shmctl(shmids[i], IPC_RMID, NULL);
  }

  // Sort the results and output to file
  auto output_file = string("../output/multi_process.txt");
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

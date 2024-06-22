#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include <config.h>

using namespace std;

int main() {
  // A single map used to count word
  auto m = unordered_map<string, int>();

  // Recursively iterate /directory_big and count word
  auto data_path = filesystem::path(REPO_PATH) / "directory_big/";
  for (auto& dir_entry : filesystem::recursive_directory_iterator(data_path)) {
    auto fin = ifstream(dir_entry.path());
    auto str = string();
    while (fin >> str) {
      m[str]++;
    }
    fin.close();
  }

  // Sort the word counting for a regularized result
  auto v = vector<pair<string, int>>();
  for (auto& [word, count] : m) {
    v.push_back({word, count});
  }
  sort(v.begin(), v.end());

  // Output to file
  auto out_path = filesystem::path(REPO_PATH) / "output/single_process.txt";
  auto fout = ofstream(out_path);
  for (auto& [word, count] : v) {
    fout << word << '\t';
    fout << count << '\n';
  }
  fout.close();
}

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <algorithm>

using namespace std;

int main() {
  auto m = unordered_map<string, int>();
  for (auto& dir_entry : filesystem::recursive_directory_iterator("../data/essay/")) {
    auto fin = ifstream(dir_entry.path());
    auto str = string();
    while (fin >> str) {
      m[str]++;
    }
    fin.close();
  }
  auto v = vector<pair<string, int>>();
  for (auto& [word, count] : m) {
    v.push_back({word, count});
  }
  sort(v.begin(), v.end());
  auto fout = ofstream("../output/single_process.txt");
  for (auto& [word, count] : v) {
    fout << word << '\t';
    fout << count << '\n';
  }
  fout.close();
}

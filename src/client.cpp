#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <config.h>

using namespace std;
namespace fs = filesystem;

vector<filesystem::path> get_files_since(const fs::path& dir, time_t t) {
  auto res = vector<filesystem::path>();
  for (auto& dir_entry : fs::recursive_directory_iterator(dir)) {
    // Convert file time to system time
    auto ftime = fs::last_write_time(dir_entry.path());
    auto sctp = decltype(ftime)::clock::to_sys(ftime);
    std::time_t file_timestamp = std::chrono::system_clock::to_time_t(sctp);
    if (file_timestamp >= t && fs::is_regular_file(dir_entry)) {
      res.push_back(dir_entry.path());
    }
  }
  return res;
}

int main(int argc, char** argv) {

  // Convert argv[1] (string) to time_t
  char* endptr;
  time_t input_time_stamp = strtol(argv[1], &endptr, 10);
  auto data_path = string(REPO_PATH) + "/directory_big/";
  auto files = get_files_since(fs::path(data_path), input_time_stamp);

  // Create a socket
  int client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket == -1) {
    std::cerr << "Failed to create socket" << std::endl;
    return 1;
  }

  // Server address and port
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(8888);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
    std::cerr << "Invalid address/Address not supported" << std::endl;
    return 1;
  }

  // Connect to server
  if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    std::cerr << "Connection failed" << std::endl;
    return 1;
  }

  // Send data to server
  auto message = string(REPO_PATH) + "/build/word_count ";
  for (auto& f : files) {
    message += string(f) + " ";
  }
  send(client_socket, message.c_str(), strlen(message.c_str()), 0);
  std::cout << "Command sent to execute on server: " << message << std::endl;

  // Receive response from server
  char buffer[1024] = {0};
  int valread = read(client_socket, buffer, 1024);
  std::cout << "Server response: " << buffer << std::endl;

  // Close socket
  close(client_socket);

  return 0;
}

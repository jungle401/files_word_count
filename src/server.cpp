#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>

int main(int argc, char** argv) {
  // Create a socket
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    std::cerr << "Failed to create socket" << std::endl;
    return 1;
  }

  // Prepare the sockaddr_in structure
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any address
  server_addr.sin_port = htons(8888);       // Port number

  // Bind
  if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    std::cerr << "Bind failed" << std::endl;
    return 1;
  }

  while (true) {

    // Listen
    listen(server_socket, 3);  // Maximum 3 connections in the queue

    // Accept incoming connections
    std::cout << "Server listening on port 8888..." << std::endl;
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);

    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addrlen);
    if (client_socket < 0) {
      std::cerr << "Accept failed" << std::endl;
      return 1;
    }

    // Connection established with client
    // std::cout << "Connection accepted from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << std::endl;

    // Receive data from client
    char buffer[1024] = {0};
    int valread = read(client_socket, buffer, 1024);
    // std::cout << "Received from client: " << buffer << std::endl;
    std::cout << "Message received from client" << std::endl;

    // Process data (e.g., execute your routine)
    // Example: Execute a command based on the received data
    std::string command(buffer);
    command += std::string(" ") + std::string(argv[1]);
    // std::cout << "Executing command: " << command << std::endl;
    std::cout << "Executing command...\n" << std::endl;
    system(command.c_str()); // Example: Execute the command (use with caution!)

    // Send response back to client
    const char* response = "Command executed.";
    send(client_socket, response, strlen(response), 0);
    // std::cout << "Response sent to client." << std::endl;

    close(client_socket);
  }

  // Close sockets
  close(server_socket);

  return 0;
}


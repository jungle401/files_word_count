cmake_minimum_required(VERSION 3.10)
project(InterviewYuChun VERSION 1.0.0)

# For LSP to recognize environments defined by this file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Configure a header file to pass the repository path
message(${PROJECT_BINARY_DIR})
configure_file(${PROJECT_SOURCE_DIR}/include/config.h.in ${PROJECT_SOURCE_DIR}/include/config.h)

# Specify the include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
# Add the executable
add_executable(word_count src/main.cpp)
add_executable(server src/server.cpp)
add_executable(client src/client.cpp)

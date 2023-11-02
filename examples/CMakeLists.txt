cmake_minimum_required(VERSION 3.10)
project(SocketExamples)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Add the local project module to the CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Find Rehoboam
find_package(Rehoboam REQUIRED)

# Define directories
set(SERVER_SRC_DIR "${CMAKE_SOURCE_DIR}/server")
set(CLIENT_SRC_DIR "${CMAKE_SOURCE_DIR}/client")
set(CUBE_CLIENT_SRC_DIR "${CMAKE_SOURCE_DIR}/cube")

# Output directories for executables and objects
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../bin/examples")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../build/examples")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../build/examples")

# Add executables
add_executable(SocketServer ${SERVER_SRC_DIR}/ServerExample.cpp)
add_executable(SocketClient ${CLIENT_SRC_DIR}/ClientExample.cpp)
add_executable(Cube ${CUBE_CLIENT_SRC_DIR}/CubeExample.cpp)

# Link the libraries
target_link_libraries(SocketServer PRIVATE Rehoboam::SocketLibrary OpenSSL::SSL OpenSSL::Crypto pthread dl)
target_link_libraries(SocketClient PRIVATE Rehoboam::SocketLibrary OpenSSL::SSL OpenSSL::Crypto pthread dl)
target_link_libraries(Cube PRIVATE Rehoboam::SocketLibrary OpenSSL::SSL OpenSSL::Crypto pthread dl)

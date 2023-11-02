cmake_minimum_required(VERSION 3.10)
project(ServerRelay)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Find Rehoboam
find_package(Rehoboam REQUIRED)

# Define the example executable
add_executable(${PROJECT_NAME} ServerRelay.cpp)

# Link the libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Rehoboam::SocketLibrary OpenSSL::SSL OpenSSL::Crypto)

# Specify the C++ standard
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

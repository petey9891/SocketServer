cmake_minimum_required(VERSION 3.10)
project(SocketLibrary VERSION 1.0.0)

# Add an interface target for the header-only library
add_library(${PROJECT_NAME} INTERFACE)

# Specify the C++ standard
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

# Define the include directories for the library
target_include_directories(SocketLibrary
    INTERFACE
        $<INSTALL_INTERFACE:include/Rehoboam>    
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

include(GNUInstallDirs)

set_target_properties(SocketLibrary PROPERTIES EXPORT_NAME SocketLibrary)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/RehoboamConfig.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/RehoboamConfig.cmake
               @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/RehoboamConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Rehoboam)

# Install the headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Rehoboam)

# Install the target
install(TARGETS SocketLibrary
    EXPORT RehoboamTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install the export set for use with the install-tree
install(EXPORT RehoboamTargets
    FILE RehoboamTargets.cmake
    NAMESPACE Rehoboam::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Rehoboam
)

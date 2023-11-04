cmake_minimum_required(VERSION 3.9.1)

project(VicOne)
project(MyProject VERSION 1.0)

# Include directories for header files
include_directories(${CMAKE_SOURCE_DIR}/include)
# Source files
set(SOURCE_FILES
    src/main.cpp
    src/network_utils.cpp
    src/message.cpp
    # Add other source files here
)

# Add the executable
add_executable(client ${SOURCE_FILES})
target_compile_features(client PRIVATE cxx_std_17)
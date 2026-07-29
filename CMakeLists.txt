cmake_minimum_required(VERSION 3.10)

# set the project name
project(jine)

set(CMAKE_CXX_STANDARD 11)

find_package(Threads REQUIRED)

# add the executable
add_executable(jine
	jine.cpp
)

target_link_libraries(jine PRIVATE Threads::Threads)

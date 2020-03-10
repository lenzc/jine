cmake_minimum_required(VERSION 3.10)

# set the project name
project(jine)

set(CMAKE_CXX_STANDARD 11)

# add the executable
add_executable(jine
	jine.cpp
)


add_executable(pulse_test
	pulse_test.cpp
)

target_link_libraries(pulse_test
	pulse
	pulse-simple
)

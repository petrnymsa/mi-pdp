cmake_minimum_required(VERSION 3.10)
project(mi_pdp)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "-Wall -Wextra -Wconversion -pedantic")
include_directories(/usr/include/mpi/)
add_executable(mi_pdp main.cpp src/map_info.h src/array_map.h)
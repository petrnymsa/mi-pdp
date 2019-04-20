#!/bin/bash
export OMPI_CXX=g++
echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
mpic++ main.cpp src/array_map.h src/map_info.h -Wall -pedantic -O3 -Wextra -fopenmp -o mpi.out

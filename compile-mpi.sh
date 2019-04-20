#!/bin/bash
export OMPI_CXX=g++
mpic++ main.cpp src/array_map.h src/map_info.h -fopenmp -o mpi.out

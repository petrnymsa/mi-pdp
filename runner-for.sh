#!/bin/bash

for file in ./data/*
do
    if [[ -f $file ]]; then
	result=$((time ./a.out $file) |& grep real | awk '{print $2}')
	echo "$file: $result" >> result-tasks.txt
    fi
done



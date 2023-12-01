#!/bin/bash

parent_dir="files"
mkdir -p $parent_dir

dir1Count=100
dir2Count=10000
dir3TotalCount=10000
dir3PerCount=100

echo "Creating directory 1 of $dir1Count files of each 1GB"
time ( 
    mkdir -p $parent_dir/dir1
    for ((i=1; i<=$dir1Count; i++)); do
        dd if=/dev/zero of=$parent_dir/dir1/file$i bs=1M count=1 status=none
    done
)

echo "Creating directory 2 of $dir2Count files of each 10MB"
time (
    mkdir -p $parent_dir/dir2
    for ((i=1; i<= $dir2Count; i++)); do
        dd if=/dev/zero of=$parent_dir/dir2/file$i bs=10M count=1 status=none
    done
)

echo "Creating directory 3 of $dir3TotalCount files of each 10MB"
currDir3="$parent_dir/dir3"
mkdir -p $currDir3
i=1
time (
    while [ $i -le $dir3TotalCount ]; do
    	dd if=/dev/zero of=$currDir3/file$i bs=1M count=1 status=none    	
    	if [ "$((i % dir3PerCount))" -eq 0 ] && [ $i -ne $dir3TotalCount ]; then
    		currDir3="$currDir3/subDir"
    		mkdir -p $currDir3
    	fi    	
    	((i++))
    done
)

echo "All files generated"

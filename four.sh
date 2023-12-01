#!/bin/bash

parent_dir="files"
echo "Q1"
for ((i=1; i<= 3; i++)); do
	echo "Directory $i:"
	echo "Copy"
	t1=$(date +%s%3N); ./Q1 cp "$parent_dir/dir$i" "$parent_dir/dir_copy$i" -r; t2=$(date +%s%3N); echo "$((t2-t1)) ms"
	echo "Move"
	t1=$(date +%s%3N); ./Q1 mv "$parent_dir/dir_copy$i" "$parent_dir/dir_move$i" -r; t2=$(date +%s%3N); echo "$((t2-t1)) ms"
	echo "Remove"
	t1=$(date +%s%3N); ./Q1 rm "$parent_dir/dir_move$i" -r; t2=$(date +%s%3N); echo "$((t2-t1)) ms"
done


echo "Q3 (Multithreading)"
for ((i=1; i<= 3; i++)); do
	echo "Directory $i:"
	echo "Copy"
	t1=$(date +%s%3N); ./Q3 cp "$parent_dir/dir$i" "$parent_dir/dir_copy$i" -r; t2=$(date +%s%3N); echo "$((t2-t1)) ms"
	echo "Move"
	t1=$(date +%s%3N); ./Q3 mv "$parent_dir/dir_copy$i" "$parent_dir/dir_move$i" -r; t2=$(date +%s%3N); echo "$((t2-t1)) ms"
	echo "Remove"
	t1=$(date +%s%3N); ./Q3 rm "$parent_dir/dir_move$i" -r; t2=$(date +%s%3N); echo "$((t2-t1)) ms"
done


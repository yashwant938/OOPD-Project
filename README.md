# OOPD-Project
Project contain four questions

# Linux Shell Implementation and Profiling

## Task 1: Basic Implementation of Commands

### Description:
Implemented a custom Linux shell that supports five basic commands: cd, mv, rm, ls, and cp. The implementation includes additional options for each command, such as recursive and help options. The shell utilizes classes and objects, supporting wildcard characters as well.

### Features:
1. **cd:** Change directory
2. **mv:** Move files or directories
3. **rm:** Remove files or directories
4. **ls:** List files in a directory
5. **cp:** Copy files or directories

### Additional Options:
- Recursive: Implemented wherever applicable for relevant commands
- Help: Provides information about command usage and available options
- and other then these four features implemented

## Task 2: Profiling

### Description:
Created a bash script to generate different directory structures with varying file sizes and quantities. Used the `time` command to profile the execution time of each command for different directory structures.

### Directory Structures:
1. 100 files of 1GB each
2. 10,000 files of 10MB each
3. 100 files of 10MB each with recursive subdirectories up to 10,000 files

### Profiling Results:
- Command execution time for each directory structure

## Task 3: Multi-threaded Implementation

### Description:
Implemented a multi-threaded version for commands involving recursion. The number of threads is dynamically determined based on available CPU cores and workload. The implementation uses the multi-threading library and employs a mutex lock for proper synchronization.

### Multi-threaded Commands:
- Recursive versions of mv, rm, cp

## Task 4: Profiling of Multi-threaded Version

### Description:
Re-profiled the multi-threaded version and compared its performance with the previous single-threaded implementation.

### Results:
- Comparative analysis of execution time between single-threaded and multi-threaded versions for recursive commands

---

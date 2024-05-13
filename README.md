# pipelined-grep
This repository contains my program, pipegrep.cpp, that searches the current directory for a file containing a specified string. The user is able to filter files out of the search based on file size, file user id, and file group id.
PipeGrep is a multi-threaded C++ program designed to search for a given string in files within a directory. It utilizes a pipeline architecture with multiple stages, each performing specific tasks such as file filtering, line generation, line filtering, and output. The program aims to efficiently search for the specified string across multiple files using parallelism.

## Features Implemented

1. Filename Acquisition (Stage 1): The program successfully acquires filenames from the current directory.

2. File Filter (Stage 2): Filters files based on criteria such as filesize, user ID (uid), and group ID (gid).

3. Line Generation (Stage 3): Reads lines from the filtered files and associates each line with its filename.

4. Line Filter (Stage 4): Filters lines containing the specified search string and counts the number of matches found.

5. Output (Stage 5): Outputs the matched lines along with their filenames.

## Critical Sections

Critical sections in the program are where shared resources, such as bounded buffers and atomic variables, are accessed or modified by multiple threads simultaneously.
This program sections are protected using mutex locks to ensure thread safety and prevent data races. Critical sections include:

- Accessing and modifying bounded buffers in the produce and consume methods of the BoundedBuffer class.
- Incrementing the matchCount atomic variable in Stage 4 (stage4 function).

## Buffer Size for Optimal Performance

After experimentation, I was unable to find a definitive buffer size that consistently yielded a faster execution time. I tested my program by using a clock within the <chrono> library, but for the results of every test run varied no matter what buffer size I used.

## Additional Thread for Performance Improvement

To further improve performance, an additional thread could be added in Stage 3, responsible for reading lines from files and populating the
buffer. This addition would parallelize the reading process, especially beneficial when dealing with large files or a large number of files.


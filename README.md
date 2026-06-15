## Memory Allocator in C

This library allocates and manages memory in a C application. 
It requests memory from the OS by using the mmap() system call and keeps track of 
available memory by using a linked list (free list data structure).

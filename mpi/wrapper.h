#pragma once

#define _GNU_SOURCE 1
#include <unistd.h>
#include <sys/mman.h>

#include <stdio.h>

// Wrappers for various syscalls.
// Checks return value and prints any errors.

void *mremap_wrapper(void *old_address, size_t old_size, size_t new_size, int flags);
void *mmap_wrapper(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
void ftruncate_wrapper(int fd, off_t length);
void fread_wrapper(void *ptr, size_t size, size_t nmemb, FILE *stream);

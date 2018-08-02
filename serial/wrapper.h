#pragma once

#include <stdio.h>

// Wrappers for various syscalls.
// Checks return value and prints any errors.

void fread_wrapper(void *ptr, size_t size, size_t nmemb, FILE *stream);

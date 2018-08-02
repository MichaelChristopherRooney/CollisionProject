#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "wrapper.h"

// Wrappers for various syscalls.
// Checks return value and prints any errors.

void fread_wrapper(void *ptr, size_t size, size_t nmemb, FILE *stream){
	size_t res = fread(ptr, size, nmemb, stream);
	if(res == 0){
		int e = ferror(stream);
		if(e != 0){ // 0 means eof, which is fine
			printf("fread failed.\n");
			exit(1);
		}
	}
}

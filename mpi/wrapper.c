#include "wrapper.h" // first due to include order requirement

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "mpi_vars.h"

// Wrappers for various syscalls.
// Checks return value and prints any errors.

void *mremap_wrapper(void *old_address, size_t old_size, size_t new_size, int flags){
	void *new_addr = mremap(old_address, old_size, new_size, flags);
	if(new_addr == NULL || new_addr == (void *) -1){
		printf("mremap failed on node %d\n", GRID_RANK);
		printf("old address: %p, old size: %ld, new size: %ld, flags: %d\n", old_address, old_size, new_size, flags);
		printf("Errno message: %s\n", strerror(errno));
		exit(1);
	}
	return new_addr;
}

void *mmap_wrapper(void *addr, size_t length, int prot, int flags, int fd, off_t offset){
	void *new_addr = mmap(addr, length, prot, flags, fd, offset);	
	if(new_addr == NULL || new_addr == (void *) -1){
		printf("mmap failed on node %d\n", GRID_RANK);
		printf("addr: %p, length: %ld, prot: %d, flags: %d, fd: %d, offset: %ld\n", addr, length, prot, flags, fd, offset);
		printf("Errno message: %s\n", strerror(errno));
		exit(1);
	}
	return new_addr;
}

void ftruncate_wrapper(int fd, off_t length){
	int res = ftruncate(fd, length);
	if(res == -1){
		printf("mmap failed on node %d\n", GRID_RANK);
		printf("fd: %d, length: %ld\n", fd, length);
		printf("Errno message: %s\n", strerror(errno));
		exit(1);
	}
}

void fread_wrapper(void *ptr, size_t size, size_t nmemb, FILE *stream){
	size_t res = fread(ptr, size, nmemb, stream);
	if(res == 0){
		int e = ferror(stream);
		if(e != 0){ // 0 means eof, which is fine
			printf("fread failed on node %d\n", GRID_RANK);
			exit(1);
		}
	}
}

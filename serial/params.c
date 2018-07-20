#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "params.h"
#include "sector.h"

static void set_default_params(){
	SECTOR_DIMS[0] = 1;
	SECTOR_DIMS[1] = 1;
	SECTOR_DIMS[2] = 1;
	final_state_file = NULL;
	visualiser_output_file = NULL;
}

static void check_slice_arg(int slice, char axis){
	if(slice <= 0){
		printf("Error: %c axis needs at least 1 slice.\n", axis);
		exit(1);
	}
}

static void print_config(){
	printf("Number of x slices: %d\n", SECTOR_DIMS[0]);
	printf("Number of y slices: %d\n", SECTOR_DIMS[1]);
	printf("Number of z slices: %d\n", SECTOR_DIMS[2]);
}

void parse_args(int argc, char *argv[]) {
	set_default_params();
	int c;
	while((c = getopt(argc, argv, "x:y:z:")) != -1) {
		switch(c) {
		case 'x':
			SECTOR_DIMS[0] = atoi(optarg);
			check_slice_arg(SECTOR_DIMS[0], 'x');
			break;
		case 'y':
			SECTOR_DIMS[1] = atoi(optarg);
			check_slice_arg(SECTOR_DIMS[1], 'y');
			break;
		case 'z':
			SECTOR_DIMS[2] = atoi(optarg);
			check_slice_arg(SECTOR_DIMS[2], 'z');
			break;
		}
	}
	print_config();
}

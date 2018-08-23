#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "simulation.h"
#include "mpi_vars.h"
#include "params.h"
#include "sector.h"

static void set_default_params(){
	sim_data.sector_dims[0] = 1;
	sim_data.sector_dims[1] = 1;
	sim_data.sector_dims[2] = 1;
	sim_data.time_limit = 0.0;
	initial_state_file = NULL;
	final_state_file = NULL;
	compare_file = NULL;
	output_file = NULL;
	ALL_HELP = false;
}

static void check_dim_arg(int slice, char axis){
	if(slice <= 0){
		if(WORLD_RANK == 0){
			printf("Error: %c axis needs at least 1 slice.\n", axis);
		}
		MPI_Finalize();
		exit(1);
	}
}

static void print_config(){
	if(WORLD_RANK != 0){
		return;
	}
	printf("Number of x slices: %d\n", sim_data.sector_dims[0]);
	printf("Number of y slices: %d\n", sim_data.sector_dims[1]);
	printf("Number of z slices: %d\n", sim_data.sector_dims[2]);
	printf("Time limit: %f\n", sim_data.time_limit);
	printf("Initial state file: %s\n", initial_state_file);
	printf("Output file: %s\n", output_file);
	if(final_state_file != NULL){
		printf("Final state file: %s\n", final_state_file);
	} else {
		printf("Final state file not set.\n");
	}
	if(compare_file != NULL){
		printf("Compare file set to: %s\n", compare_file);
	} else {
		printf("Compare file not set\n");
	}
	if(ALL_HELP){
		printf("ALL_HELP is set.\nAll nodes will find events for sectors without valid prior times\n");
	} else {
		printf("ALL_HELP is NOT set.\nOnly neighbour nodes will find events for sectors without valid prior times\n");
	}
}

static void validate_args(){
	check_dim_arg(sim_data.sector_dims[X_AXIS], 'x');
	check_dim_arg(sim_data.sector_dims[Y_AXIS], 'y');
	check_dim_arg(sim_data.sector_dims[Z_AXIS], 'z');
	if(sim_data.time_limit <= 0.0){
		if(WORLD_RANK == 0){
			printf("Error: time limit should be > 0\n");
		}
		MPI_Finalize();
		exit(1);
	}
	if(sim_data.sector_dims[X_AXIS] * sim_data.sector_dims[Y_AXIS] * sim_data.sector_dims[Z_AXIS] != NUM_NODES){
		if(WORLD_RANK == 0){
			printf("Error: number of sectors should match number of nodes\n");
		}
		MPI_Finalize();
		exit(1);
	}
	if(output_file == NULL){
		if(WORLD_RANK == 0){
			printf("Error: output file (-o) cannot be null\n");
		}
		MPI_Finalize();
		exit(1);
	}
	if(initial_state_file == NULL){
		if(WORLD_RANK == 0){
			printf("Error: initial state file (-i) cannot be null\n");
		}
		MPI_Finalize();
		exit(1);
	}
}
	
static void print_help(){
	if(WORLD_RANK == 0){
		printf("-x, -y, -z:\n");
		printf("\tOptional.\n\tSets the number of slices in the specified axis.\n\tDefaults to 1.\n\tMust be at least 1.\n");
		printf("-c:\n\tOptional.\n\tSets the compare file. This should be a final state file from a previous run.\n\tThe max error between this run and the compare file will be printed.\n");
		printf("-f:\n\tOptional.\n\tSets the final state file. This will contain only the final velocity and position of each sphere.\n");
		printf("-o:\n\tRequired.\n\tSets the output file. This contains all data needed to make use of the simulation.\n");
		printf("-i:\n\tRequired.\n\tSets the inital state file.\n");
		printf("-t:\n\tRequired.\n\tSets the time the simulation will run for.\n");
	}
	MPI_Finalize();
	exit(0);
}

void parse_args(int argc, char *argv[]) {
	set_default_params();
	int c;
	while((c = getopt(argc, argv, "ai:c:f:ho:x:y:z:t:")) != -1) {
		switch(c) {
		case 'a':
			ALL_HELP = true;
			break;
		case 'x':
			sim_data.sector_dims[X_AXIS] = atoi(optarg);
			break;
		case 'y':
			sim_data.sector_dims[Y_AXIS] = atoi(optarg);
			break;
		case 'z':
			sim_data.sector_dims[Z_AXIS] = atoi(optarg);
			break;
		case 'f':
			final_state_file = optarg;
			break;
		case 'o':
			output_file = optarg;
			break;
		case 'c':
			compare_file = optarg;
			break;
		case 'h':
			print_help();
			break;
		case 'i':
			initial_state_file = optarg;
			break;
		case 't':
			sim_data.time_limit = atof(optarg);
			break;
		}
	}
	validate_args();
	print_config();
}

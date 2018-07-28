#include "io.h"

#include "grid.h"
#include "mpi_vars.h"
#include "simulation.h"

// Skips grid x/y/z size (doubles) in file
static const int64_t base_offset = (sizeof(double) * 3) + sizeof(int64_t);

// How much room each sphere takes up in the file.
// 64 bit id + velocity and position for x/y/z as doubles.
static const int64_t sphere_file_size = sizeof(int64_t) + (sizeof(double) * 6);

// Iteration number, time and number of spheres in the iteration.
static const iteration_header_size = sizeof(double) + sizeof(int64_t) + sizeof(int64_t);

// Due to what seems like a bug with OpenMPI using MPI_SEEK_CUR sets the file
// pointer to the offset rather than adding the offset to it.
// To get around this the offset is tracked manually and MPI_SEEK_SET is used.
static int64_t cur_file_offset = 0; 

static int64_t radius_mass_block_size;

// File format looks like this:
// Grid x, y, and z size
// For each sphere: radius and mass
// Iteration number (0 since initial state)
// For each sphere: id, velocity for x, y and z, position for x, y and z
// Because we are loading spheres and discarding them if they don't belong
// to the local node we write this inital data here for spheres may be discarded.
// Because the radius/mass are stored away from the rest of the inital data we
// need to seek to the correct offset for each sphere.
void write_sphere_initial_state(const struct sphere_s *sphere){
	if(GRID_RANK != 0){
		return;
	}
	int64_t radius_mass_offset = base_offset + (sphere->id * (sizeof(double) * 2));
	int64_t sphere_offset = base_offset + radius_mass_block_size + iteration_header_size + (sphere_file_size * sphere->id);
	MPI_File_seek(MPI_OUTPUT_FILE, radius_mass_offset, MPI_SEEK_SET);
	MPI_Status s;
	MPI_File_write(MPI_OUTPUT_FILE, &sphere->radius, 1, MPI_DOUBLE, &s);
	MPI_File_write(MPI_OUTPUT_FILE, &sphere->mass, 1, MPI_DOUBLE, &s);
	MPI_File_seek(MPI_OUTPUT_FILE, sphere_offset, MPI_SEEK_SET);
	MPI_File_write(MPI_OUTPUT_FILE, &sphere->id, 1, MPI_LONG_LONG, &s);
	MPI_File_write(MPI_OUTPUT_FILE, &sphere->vel, 3, MPI_DOUBLE, &s);
	MPI_File_write(MPI_OUTPUT_FILE, &sphere->pos, 3, MPI_DOUBLE, &s);
}

// After initial state has been writen go back and write initial iteration data.
// This should have iteration number = 0, time = 0 and number of spheres = NUM_SPHERES.
void write_initial_iteration_stats(){
	if(GRID_RANK == 0){
		MPI_File_seek(MPI_OUTPUT_FILE, base_offset + radius_mass_block_size, MPI_SEEK_SET);
		int64_t i = 0;
		double t = 0;
		MPI_Status s;
		MPI_File_write(MPI_OUTPUT_FILE, &i, 1, MPI_LONG_LONG, &s);
		MPI_File_write(MPI_OUTPUT_FILE, &t, 1, MPI_DOUBLE, &s);
		MPI_File_write(MPI_OUTPUT_FILE, &NUM_SPHERES, 1, MPI_LONG_LONG, &s);
	}
	cur_file_offset = base_offset + radius_mass_block_size + iteration_header_size + (sphere_file_size * NUM_SPHERES);
	MPI_File_seek(MPI_OUTPUT_FILE, cur_file_offset, MPI_SEEK_SET);
}

void write_num_spheres(){
	radius_mass_block_size = (NUM_SPHERES * (sizeof(double) * 2));
	if(GRID_RANK != 0){
		return;
	}
	MPI_Status s;
	MPI_File_write(MPI_OUTPUT_FILE, &NUM_SPHERES, 1, MPI_LONG_LONG, &s);
}

void write_grid_dimms(){
	if(GRID_RANK != 0){
		return;
	}
	MPI_Status s;
	MPI_File_write(MPI_OUTPUT_FILE, &grid->size, 3, MPI_DOUBLE, &s);
}

void write_iteration_data(struct sphere_s *s1, struct sphere_s *s2){
	MPI_Status s;
	double t = grid->elapsed_time + next_event->time;
	MPI_File_write(MPI_OUTPUT_FILE, &ITERATION_NUMBER, 1, MPI_LONG_LONG, &s);
	MPI_File_write(MPI_OUTPUT_FILE, &t, 1, MPI_DOUBLE, &s);
	int64_t n;
	if(s2 != NULL){
		n = 2;
	} else {
		n = 1;
	}
	MPI_File_write(MPI_OUTPUT_FILE, &n, 1, MPI_LONG_LONG, &s);
	MPI_File_write(MPI_OUTPUT_FILE, &s1->id, 1, MPI_LONG_LONG, &s);
	MPI_File_write(MPI_OUTPUT_FILE, &s1->vel, 3, MPI_DOUBLE, &s);
	MPI_File_write(MPI_OUTPUT_FILE, &s1->pos, 3, MPI_DOUBLE, &s);
	cur_file_offset += iteration_header_size + sphere_file_size;
	if(s2 != NULL){
		MPI_File_write(MPI_OUTPUT_FILE, &s2->id, 1, MPI_LONG_LONG, &s);
		MPI_File_write(MPI_OUTPUT_FILE, &s2->vel, 3, MPI_DOUBLE, &s);
		MPI_File_write(MPI_OUTPUT_FILE, &s2->pos, 3, MPI_DOUBLE, &s);
		cur_file_offset += sphere_file_size;
	}
}

void seek_one_sphere(){
	cur_file_offset += iteration_header_size + sphere_file_size;
	MPI_File_seek(MPI_OUTPUT_FILE, cur_file_offset, MPI_SEEK_SET);
}

void seek_two_spheres(){
	cur_file_offset += iteration_header_size + (sphere_file_size * 2);
	MPI_File_seek(MPI_OUTPUT_FILE, cur_file_offset, MPI_SEEK_SET);
}


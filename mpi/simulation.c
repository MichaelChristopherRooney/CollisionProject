#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "grid.h"
#include "params.h"
#include "vector_3.h"

static FILE *data_file;

// Saves a sphere to the file
static void save_sphere_to_file(struct sphere_s *s) {
	fwrite(&s->id, sizeof(int64_t), 1, data_file);
	fwrite(&s->vel.x, sizeof(double), 1, data_file);
	fwrite(&s->vel.y, sizeof(double), 1, data_file);
	fwrite(&s->vel.z, sizeof(double), 1, data_file);
	fwrite(&s->pos.x, sizeof(double), 1, data_file);
	fwrite(&s->pos.y, sizeof(double), 1, data_file);
	fwrite(&s->pos.z, sizeof(double), 1, data_file);
}

// Saves initial state of all spheres
// Note: need to still write count as parsers want to know how many spheres have
// changed since last iteration.
// As this is the first iteration every sphere has "changed".
static void save_sphere_initial_state_to_file() {
	uint64_t iter_num = 0;
	double time_elapsed = 0.0;
	fwrite(&iter_num, sizeof(uint64_t), 1, data_file);
	fwrite(&time_elapsed, sizeof(double), 1, data_file);
	fwrite(&NUM_SPHERES, sizeof(uint64_t), 1, data_file);
	int i;
	for (i = 0; i < NUM_SPHERES; i++) {
		save_sphere_to_file(&spheres[i]);
	}
}

// First writes the current iteration number as well as the simulation timestamp.
// Then writes the number of changed spheres to the file, followed by the data for each changed sphere.
static void save_sphere_state_to_file(uint64_t iteration_num, double time_elapsed) {
	fwrite(&iteration_num, sizeof(uint64_t), 1, data_file);
	fwrite(&time_elapsed, sizeof(double), 1, data_file);
	if (event_details.type == COL_TWO_SPHERES) {
		uint64_t count = 2;
		fwrite(&count, sizeof(uint64_t), 1, data_file);
		save_sphere_to_file(event_details.sphere_1);
		save_sphere_to_file(event_details.sphere_2);
	} else {
		uint64_t count = 1;
		fwrite(&count, sizeof(uint64_t), 1, data_file);
		save_sphere_to_file(event_details.sphere_1);
	}
}

// First write the grid's dimensions then the number of spheres.
// Then write the initial state of the spheres.
// The iteration number and the time elapsed are 0 as nothing has
// happened yet.
static void init_binary_file() {
	//printf("TODO: init binary file MPI\n");
}

// Writes the final state of the spheres.
// This can be used to compare results and ensure correctness
static void write_final_state(){
	if(final_state_file == NULL){
		return;
	}
	FILE *fp = fopen(final_state_file, "wb");
	fwrite(&NUM_SPHERES, sizeof(int64_t), 1, fp);
	int64_t i;
	for (i = 0; i < NUM_SPHERES; i++) {
		fwrite(&spheres[i].vel.x, sizeof(double), 1, fp);
		fwrite(&spheres[i].vel.y, sizeof(double), 1, fp);
		fwrite(&spheres[i].vel.z, sizeof(double), 1, fp);
		fwrite(&spheres[i].pos.x, sizeof(double), 1, fp);
		fwrite(&spheres[i].pos.y, sizeof(double), 1, fp);
		fwrite(&spheres[i].pos.z, sizeof(double), 1, fp);
	}
	fclose(fp);
}

static void compare_results() {
	if(compare_file == NULL){
		return;
	}
	FILE *fp = fopen(compare_file, "rb");
	int64_t comp_num_spheres;
	// result is just to shut up gcc's warnings
	int result = fread(&comp_num_spheres, sizeof(int64_t), 1, fp);
	if(comp_num_spheres != NUM_SPHERES){
		printf("Error: cannot compare files as number of spheres differs\n");
		return;
	}
	double max_pos_err = 0.0;
	double max_vel_err = 0.0;
	int i;
	for (i = 0; i < NUM_SPHERES; i++) {
		struct sphere_s s = spheres[i];
		union vector_3d vel_comp;
		union vector_3d pos_comp;
		result = fread(&vel_comp, sizeof(union vector_3d), 1, fp);
		result = fread(&pos_comp, sizeof(union vector_3d), 1, fp);
		enum axis a;
		for (a = X_AXIS; a <= Z_AXIS; a++) {
			if (fabs(s.pos.vals[a] - pos_comp.vals[a]) > max_pos_err) {
				max_pos_err = fabs(s.pos.vals[a] - pos_comp.vals[a]);
			}
			if (fabs(s.vel.vals[a] - vel_comp.vals[a]) > max_vel_err) {
				max_vel_err = fabs(s.vel.vals[a] - vel_comp.vals[a]);
			}
		}
	}
	printf("vel abs err: %.17g\n", max_vel_err);
	printf("pos abs err: %.17g\n", max_pos_err);
}

void simulation_init(double time_limit) {
	init_grid(time_limit);
	init_binary_file();
}

void simulation_run() {
	int i = 1; // start at 1 as 0 is iteration num for the initial state
	while (grid->elapsed_time < grid->time_limit) {
		grid->elapsed_time += update_grid();
		save_sphere_state_to_file(i, grid->elapsed_time);
		i++;
	}
	write_final_state();
	compare_results();
}

void simulation_cleanup() {
	printf("TODO: cleanup in MPI version\n");
}

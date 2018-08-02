#include <stdio.h>
#include <unistd.h>

#include "grid.h"
#include "io.h"
#include "params.h"

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
void save_sphere_state_to_file(uint64_t iteration_num, double time_elapsed) {
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
void init_binary_file() {
	data_file = fopen(output_file, "wb");
	fwrite(&grid->size.x, sizeof(double), 1, data_file);
	fwrite(&grid->size.y, sizeof(double), 1, data_file);
	fwrite(&grid->size.z, sizeof(double), 1, data_file);
	fwrite(&NUM_SPHERES, sizeof(int64_t), 1, data_file);
	int64_t i;
	for (i = 0; i < NUM_SPHERES; i++) {
		fwrite(&spheres[i].radius, sizeof(double), 1, data_file);
		fwrite(&spheres[i].mass, sizeof(double), 1, data_file);
	}
	save_sphere_initial_state_to_file();
}

// Writes the final state of the spheres.
// This can be used to compare results and ensure correctness
void write_final_state(){
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

// Old output or final state files may be present if names are reused.
// This has been causing issues so delete them here.
void delete_old_files(){
	if(final_state_file != NULL){
		unlink(final_state_file);
	}
	unlink(output_file);
}

void write_final_time_to_file(){
	fwrite(&grid->time_limit, sizeof(double), 1, data_file);
}

void close_data_file(){
	fclose(data_file);
}

#include <stdio.h>
#include <stdlib.h>

#include "grid.h"
#include "vector_3.h"

static FILE *data_file;

// Saves a sphere to the file
static void save_sphere_to_file(struct sphere_s *s) {
	fwrite(&s->id, sizeof(uint64_t), 1, data_file);
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
// TODO: probably need an id for each sphere
static void init_binary_file(char *fp) {
	data_file = fopen(fp, "wb");
	fwrite(&grid->start.x, sizeof(double), 1, data_file);
	fwrite(&grid->end.x, sizeof(double), 1, data_file);
	fwrite(&grid->start.y, sizeof(double), 1, data_file);
	fwrite(&grid->end.y, sizeof(double), 1, data_file);
	fwrite(&grid->start.z, sizeof(double), 1, data_file);
	fwrite(&grid->end.z, sizeof(double), 1, data_file);
	uint64_t temp = NUM_SPHERES;
	fwrite(&temp, sizeof(uint64_t), 1, data_file);
	int i;
	for (i = 0; i < NUM_SPHERES; i++) {
		fwrite(&spheres[i].radius, sizeof(double), 1, data_file);
		fwrite(&spheres[i].mass, sizeof(double), 1, data_file);
	}
	save_sphere_initial_state_to_file();
}

void simulation_init(char *fp, union vector_3i *divs, union vector_3d *grid_start, union vector_3d *grid_end) {
	init_grid(divs, grid_start, grid_end);
	init_binary_file(fp);
}

void simulation_run() {
	double limit = 10.0;
	double time_elapsed = 0;
	int i = 1; // start at 1 as 0 is iteration num for the initial state
	while (time_elapsed < limit) {
		time_elapsed += update_grid(limit, time_elapsed);
		save_sphere_state_to_file(i, time_elapsed);
		i++;
	}
}

void simulation_cleanup() {
	if (grid->uses_sectors) {
		free(grid->sectors[0][0]);
		int i;
		for (i = 0; i < SECTOR_DIMS[X_AXIS]; i++) {
			free(grid->sectors[i]);
		}
		free(grid->sectors);
	}
	free(spheres);
	fclose(data_file);
	free(grid);
}
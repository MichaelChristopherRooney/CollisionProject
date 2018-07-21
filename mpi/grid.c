#include <float.h>
#include <stdlib.h>
#include <stdio.h>

#include "collision.h"
#include "grid.h"
#include "mpi_vars.h"
#include "params.h"
#include "vector_3.h"

static FILE *initial_state_fp;

// Loads spheres from the specified inital state file
// This file contains every sphere, and we need to check if the sphere belongs
// to the sector the current MPI node is responsible for.
static void load_spheres() {
	// result is just to shut up gcc's warnings
	// TODO: get rid of this global and use grid->total_spheres where needed
	int result = fread(&NUM_SPHERES, sizeof(int64_t), 1, initial_state_fp);
	struct sphere_s in;
	int64_t i;
	for(i = 0; i < NUM_SPHERES; i++){
		result = fread(&in.id, sizeof(int64_t), 1, initial_state_fp);
		result = fread(&in.pos.x, sizeof(double), 1, initial_state_fp);
		result = fread(&in.pos.y, sizeof(double), 1, initial_state_fp);
		result = fread(&in.pos.z, sizeof(double), 1, initial_state_fp);
		result = fread(&in.vel.x, sizeof(double), 1, initial_state_fp);
		result = fread(&in.vel.y, sizeof(double), 1, initial_state_fp);
		result = fread(&in.vel.z, sizeof(double), 1, initial_state_fp);
		result = fread(&in.mass, sizeof(double), 1, initial_state_fp);
		result = fread(&in.radius, sizeof(double), 1, initial_state_fp);
		if(does_sphere_belong_to_sector(&in, SECTOR)){
			//printf("Rank %d claiming sphere at %f, %f, %f\n", RANK, in.pos.x, in.pos.y, in.pos.z);
			// TODO: add sphere in a fast way
		}
	}
}

static void init_sector() {
	double x_inc = grid->size.x / SECTOR_DIMS[X_AXIS];
	double y_inc = grid->size.y / SECTOR_DIMS[Y_AXIS];
	double z_inc = grid->size.z / SECTOR_DIMS[Z_AXIS];
	SECTOR = calloc(sizeof(struct sector_s), 1);
	SECTOR->num_spheres = 0; 
	SECTOR->max_spheres = 1000; // will be increased later if needed
	SECTOR->spheres = calloc(SECTOR->max_spheres, sizeof(struct sphere_s *)); // realloc'd later if needed
	SECTOR->pos.x = COORDS[X_AXIS];
	SECTOR->pos.y = COORDS[Y_AXIS];
	SECTOR->pos.z = COORDS[Z_AXIS];
	SECTOR->start.x = x_inc * COORDS[X_AXIS];
	SECTOR->end.x = SECTOR->start.x + x_inc;
	SECTOR->start.y = y_inc * COORDS[Y_AXIS];
	SECTOR->end.y = SECTOR->start.y + y_inc;
	SECTOR->start.z = z_inc * COORDS[Z_AXIS];
	SECTOR->end.z = SECTOR->start.z + z_inc;
	printf("Rank %d handling sector with location:\n", RANK);
	printf("x: %f to %f\n", SECTOR->start.x, SECTOR->end.x);
	printf("y: %f to %f\n", SECTOR->start.y, SECTOR->end.y);
	printf("z: %f to %f\n", SECTOR->start.z, SECTOR->end.z);
	printf("TODO: init neighbouring sectors\n");
}

// Loads the grid from the initial state file
void init_grid(double time_limit) {
	initial_state_fp = fopen(initial_state_file, "rb");
	grid = calloc(1, sizeof(struct grid_s));
	// result is just to shut up gcc's warnings
	int result = fread(&grid->size.x, sizeof(double), 1, initial_state_fp);
	result = fread(&grid->size.y, sizeof(double), 1, initial_state_fp);
	result = fread(&grid->size.z, sizeof(double), 1, initial_state_fp);
	init_sector();
	load_spheres();
	grid->elapsed_time = 0.0;
	grid->time_limit = time_limit;
	grid->num_two_sphere_collisions = 0;
	grid->num_grid_collisions = 0;
	grid->num_sector_transfers = 0;
	fclose(initial_state_fp);
}

// This updates the positions and velocities of each sphere once the next
// event and the time it occurs are known.
static void update_spheres() {
	int i;
	for (i = 0; i < NUM_SPHERES; i++) {
		struct sphere_s *s = &(spheres[i]);
		update_sphere_position(s, event_details.time);
	}
	if (event_details.type == COL_SPHERE_WITH_GRID) {
		event_details.sphere_1->vel.vals[event_details.grid_axis] *= -1.0;
		grid->num_grid_collisions++;
	} else if (event_details.type == COL_TWO_SPHERES) {
		apply_bounce_between_spheres(event_details.sphere_1, event_details.sphere_2);
		grid->num_two_sphere_collisions++;
	} else if (event_details.type == COL_SPHERE_WITH_SECTOR) {
		remove_sphere_from_sector(event_details.source_sector, event_details.sphere_1);
		add_sphere_to_sector(event_details.dest_sector, event_details.sphere_1);
		grid->num_sector_transfers++;
	}
}

// For debugging
// Helps catch any issues with transfering spheres between sectors.
static void sanity_check() {
	printf("TODO: sanity check MPI\n");
}

double update_grid() {
	//sanity_check();
	// First reset records.
	event_details.time = DBL_MAX;
	event_details.sphere_1 = NULL;
	event_details.sphere_2 = NULL;
	event_details.source_sector = NULL;
	event_details.dest_sector = NULL;
	event_details.type = COL_NONE;
	event_details.grid_axis = AXIS_NONE;
	// Now find event + time of event
	printf("TODO: find sector events MPI\n");
	// Final event may take place after time limit, so cut it short
	if (grid->time_limit - grid->elapsed_time < event_details.time) {
		event_details.time = grid->time_limit - grid->elapsed_time;
		event_details.type = COL_NONE;
	}
	// Lastly move forward to the next event
	update_spheres();
	//sanity_check();
	return event_details.time;
}

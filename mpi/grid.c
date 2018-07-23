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
		struct sector_s *temp = find_sector_that_sphere_belongs_to(&in);
		if(temp == SECTOR || temp->is_neighbour){
			// want to copy it if it belongs to local node or a neighbour
			add_sphere_to_sector(SECTOR, &in);
		}
	}
}

static void init_my_sector() {
	SECTOR = &grid->sectors[COORDS[X_AXIS]][COORDS[Y_AXIS]][COORDS[Z_AXIS]];
	SECTOR->num_spheres = 0;
	SECTOR->max_spheres = 2000;
	SECTOR->spheres = calloc(SECTOR->max_spheres, sizeof(struct sphere_s));
	/*printf("Rank %d handling sector with location:\n", RANK);
	printf("x: %f to %f\n", SECTOR->start.x, SECTOR->end.x);
	printf("y: %f to %f\n", SECTOR->start.y, SECTOR->end.y);
	printf("z: %f to %f\n", SECTOR->start.z, SECTOR->end.z);
	printf("TODO: init neighbouring sectors\n");*/
}

static void alloc_sector_array(){
	grid->sectors = calloc(SECTOR_DIMS[X_AXIS], sizeof(struct sector_s **));
	struct sector_s *z_arr = calloc(SECTOR_DIMS[X_AXIS] * SECTOR_DIMS[Y_AXIS] * SECTOR_DIMS[Z_AXIS], sizeof(struct sector_s));
	int i, j;
	for (i = 0; i < SECTOR_DIMS[X_AXIS]; i++) {
		grid->sectors[i] = calloc(SECTOR_DIMS[Y_AXIS], sizeof(struct sector_s *));
		for (j = 0; j < SECTOR_DIMS[Y_AXIS]; j++) {
			int idx = (i * SECTOR_DIMS[Y_AXIS] * SECTOR_DIMS[Z_AXIS]) + (j * SECTOR_DIMS[Z_AXIS]);
			grid->sectors[i][j] = &z_arr[idx];
		}
	}
}

static void init_sectors(){
	grid->xy_check_needed = SECTOR_DIMS[X_AXIS] > 1 && SECTOR_DIMS[Y_AXIS] > 1;
	grid->xz_check_needed = SECTOR_DIMS[X_AXIS] > 1 && SECTOR_DIMS[Z_AXIS] > 1;
	grid->yz_check_needed = SECTOR_DIMS[Y_AXIS] > 1 && SECTOR_DIMS[Z_AXIS] > 1;
	alloc_sector_array();
	double x_inc = grid->size.x / SECTOR_DIMS[X_AXIS];
	double y_inc = grid->size.y / SECTOR_DIMS[Y_AXIS];
	double z_inc = grid->size.z / SECTOR_DIMS[Z_AXIS];
	int i, j, k;
	for (i = 0; i < SECTOR_DIMS[X_AXIS]; i++) {
		for (j = 0; j < SECTOR_DIMS[Y_AXIS]; j++) {
			for (k = 0; k < SECTOR_DIMS[Z_AXIS]; k++) {
				struct sector_s *s = &grid->sectors[i][j][k];
				s->start.x = x_inc * i;
				s->end.x = s->start.x + x_inc;
				s->start.y = y_inc * j;
				s->end.y = s->start.y + y_inc;
				s->start.z = z_inc * k;
				s->end.z = s->start.z + z_inc;
				s->pos.x = i;
				s->pos.y = j;
				s->pos.z = k;
			}
		}
	}
	
}

// Check if the passed sector is a neighbour to the local sector
// It's a neighbour if it is within 0 or 1 unit in all directions.
static bool check_is_neighbour(struct sector_s *s){
	int x_dist = abs(SECTOR->pos.x - s->pos.x);
	int y_dist = abs(SECTOR->pos.y - s->pos.y);
	int z_dist = abs(SECTOR->pos.z - s->pos.z);
	return x_dist <= 1 && y_dist <= 1 && z_dist <= 1;
}

static bool set_neighbours(){
	int i, j, k;
	for (i = 0; i < SECTOR_DIMS[X_AXIS]; i++) {
		for (j = 0; j < SECTOR_DIMS[Y_AXIS]; j++) {
			for (k = 0; k < SECTOR_DIMS[Z_AXIS]; k++) {
				struct sector_s *s = &grid->sectors[i][j][k];
				if(SECTOR == s){
					continue; // skip local sector
				}
				if(check_is_neighbour(s) == false){
					continue;
				}
				s->is_neighbour = true;
				s->num_spheres = 0;
				s->max_spheres = 2000;
				s->spheres = calloc(s->max_spheres, sizeof(struct sphere_s));
			}
		}
	}
}

// Loads the grid from the initial state file
void init_grid(double time_limit) {
	initial_state_fp = fopen(initial_state_file, "rb");
	grid = calloc(1, sizeof(struct grid_s));
	// result is just to shut up gcc's warnings
	int result = fread(&grid->size.x, sizeof(double), 1, initial_state_fp);
	result = fread(&grid->size.y, sizeof(double), 1, initial_state_fp);
	result = fread(&grid->size.z, sizeof(double), 1, initial_state_fp);
	init_sectors();
	init_my_sector();
	set_neighbours();
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

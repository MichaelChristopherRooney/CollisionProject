#include <float.h>
#include <stdlib.h>
#include <stdio.h>

#include "collision.h"
#include "event.h"
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
			add_sphere_to_sector(temp, &in);
		}
	}
}

static void init_my_sector() {
	SECTOR = &grid->sectors[COORDS[X_AXIS]][COORDS[Y_AXIS]][COORDS[Z_AXIS]];
	SECTOR->num_spheres = 0;
	SECTOR->max_spheres = 2000;
	SECTOR->spheres = calloc(SECTOR->max_spheres, sizeof(struct sphere_s));
	/*printf("Rank %d sector id %d\n", GRID_RANK, SECTOR->id);
	printf("Rank %d handling sector with location:\n", GRID_RANK);
	printf("x: %f to %f\n", SECTOR->start.x, SECTOR->end.x);
	printf("y: %f to %f\n", SECTOR->start.y, SECTOR->end.y);
	printf("z: %f to %f\n", SECTOR->start.z, SECTOR->end.z);
	printf("TODO: init neighbouring sectors\n");*/
}

static void alloc_sector_array(){
	grid->sectors = calloc(SECTOR_DIMS[X_AXIS], sizeof(struct sector_s **));
	grid->sectors_flat = calloc(SECTOR_DIMS[X_AXIS] * SECTOR_DIMS[Y_AXIS] * SECTOR_DIMS[Z_AXIS], sizeof(struct sector_s));
	int i, j;
	for (i = 0; i < SECTOR_DIMS[X_AXIS]; i++) {
		grid->sectors[i] = calloc(SECTOR_DIMS[Y_AXIS], sizeof(struct sector_s *));
		for (j = 0; j < SECTOR_DIMS[Y_AXIS]; j++) {
			int idx = (i * SECTOR_DIMS[Y_AXIS] * SECTOR_DIMS[Z_AXIS]) + (j * SECTOR_DIMS[Z_AXIS]);
			grid->sectors[i][j] = &grid->sectors_flat[idx];
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
	int id = 0;
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
				s->id = id;
				id++;
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
	NUM_NEIGHBOURS = 0;
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
				NEIGHBOUR_IDS[NUM_NEIGHBOURS] = s->id;
				NUM_NEIGHBOURS++;
			}
		}
	}
	if(NUM_NEIGHBOURS < MAX_NEIGHBOURS){
		NEIGHBOUR_IDS[NUM_NEIGHBOURS + 1] = -1;
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
	init_events();
	grid->elapsed_time = 0.0;
	grid->time_limit = time_limit;
	grid->num_two_sphere_collisions = 0;
	grid->num_grid_collisions = 0;
	grid->num_sector_transfers = 0;
	fclose(initial_state_fp);
}

// This updates the positions and velocities of each sphere once the next
// event and the time it occurs are known.
// First update own spheres, then update local copy of neighbour's spheres
static void update_spheres() {
	int i, j;
	for (i = 0; i < SECTOR->num_spheres; i++) {
		struct sphere_s *s = &(SECTOR->spheres[i]);
		update_sphere_position(s, next_event->time);
	}
	for(i = 0; i < NUM_NEIGHBOURS; i++){
		struct sector_s *sector = &grid->sectors_flat[NEIGHBOUR_IDS[i]];
		for(j = 0; j < sector->num_spheres; j++){
			struct sphere_s *s = &(sector->spheres[j]);
			update_sphere_position(s, next_event->time);
		}
	}
}

static void apply_event(){
	if (next_event->type == COL_SPHERE_WITH_GRID) {
		struct sector_s *source = &grid->sectors_flat[next_event->source_sector_id];
		if(source->is_neighbour || source == SECTOR->id){
			struct sphere_s *sphere = &source->spheres[next_event->sphere_1.sector_id];
			sphere->vel.vals[event_details.grid_axis] *= -1.0;
		}
	} else if (next_event->type == COL_TWO_SPHERES) {
		struct sector_s *source = &grid->sectors_flat[next_event->source_sector_id];
		if(source->is_neighbour || SECTOR->id == next_event->source_sector_id){
			struct sphere_s *s1 = &source->spheres[next_event->sphere_1.sector_id];
			struct sphere_s *s2 = &source->spheres[next_event->sphere_2.sector_id];
			apply_bounce_between_spheres(s1, s2);
		}
	} else if (next_event->type == COL_SPHERE_WITH_SECTOR) {
		struct sphere_s *sphere = &next_event->sphere_1;
		struct sector_s *source = &grid->sectors_flat[next_event->source_sector_id];
		if(source->is_neighbour || SECTOR->id == next_event->source_sector_id){
			remove_sphere_from_sector(source, sphere);
		}
		struct sector_s *dest = &grid->sectors_flat[next_event->dest_sector_id];
		if(dest->is_neighbour || SECTOR->id == next_event->dest_sector_id){
			update_sphere_position(sphere, next_event->time); // received sphere data has old pos
			add_sphere_to_sector(dest, sphere);
		}
	} else if(next_event->type == COL_TWO_SPHERES_PARTIAL_CROSSING){
		struct sphere_s *s1;
		struct sphere_s *s2;
		struct sector_s *source = &grid->sectors_flat[next_event->source_sector_id];
		struct sector_s *dest = &grid->sectors_flat[next_event->dest_sector_id];
		// If source and dest are neighbours to the local or are the local node then bounce both spheres.
		// If one of them but not the other is a neighbour or the local node then bounce the relevant sphere
		// using the copied data in next_event.
		// This will overwrite the copy which is fine.
		if((source->is_neighbour || SECTOR->id == next_event->source_sector_id) && (dest->is_neighbour || SECTOR->id == next_event->dest_sector_id)){
			s1 = &source->spheres[next_event->sphere_1.sector_id];
			s2 = &dest->spheres[next_event->sphere_2.sector_id];
			apply_bounce_between_spheres(s1, s2);
		} else if(source->is_neighbour || SECTOR->id == next_event->source_sector_id){
			s1 = &source->spheres[next_event->sphere_1.sector_id];
			s2 = &next_event->sphere_2;
			apply_bounce_between_spheres(s1, s2);
		} else if(dest->is_neighbour || SECTOR->id == next_event->dest_sector_id){
			s1 = &next_event->sphere_1;
			s2 = &dest->spheres[next_event->sphere_2.sector_id];
			apply_bounce_between_spheres(s1, s2);
		}
	}
}

// For debugging
// Helps catch any issues with transfering spheres between sectors.
static void sanity_check() {
	int i;
	for(i = 0; i < SECTOR->num_spheres; i++){
		struct sphere_s *sphere = &SECTOR->spheres[i];
		int error = 0;
		enum axis a;
		for (a = X_AXIS; a <= Z_AXIS; a++) {
			if (sphere->pos.vals[a] > SECTOR->end.vals[a]) {
				error = 1;
			}
			if (sphere->pos.vals[a] < SECTOR->start.vals[a]) {
				error = 1;
			}
		}
		if (error) {
			printf("Sector at %d, %d, %d incorrectly has sphere with pos %f, %f, %f\n", SECTOR->pos.x, SECTOR->pos.y, SECTOR->pos.z, sphere->pos.x, sphere->pos.y, sphere->pos.z);
			exit(1);
		}
	}
}

double update_grid() {
	//printf("%d: I have %ld spheres\n", GRID_RANK, SECTOR->num_spheres);
	sanity_check();
	// First reset records.
	reset_event_details();
	// Now find event + time of event
	// Final event may take place after time limit, so cut it short
	find_event_times_for_sector(SECTOR);
	reduce_events();
	if (grid->time_limit - grid->elapsed_time < next_event->time) {
		next_event->time = grid->time_limit - grid->elapsed_time;
		update_spheres();
	} else {
		update_spheres();
		apply_event();
	}
	//Lastly move forward to the next event

	//printf("%d: I have %ld spheres\n", GRID_RANK, SECTOR->num_spheres);
	sanity_check();
	return next_event->time;
}

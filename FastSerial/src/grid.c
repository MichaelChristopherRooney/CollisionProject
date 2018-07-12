#include <float.h>
#include <stdlib.h>
#include <stdio.h>

#include "collision.h"
#include "grid.h"

static int count = 0; // number of spheres that have been initialised

// Creates a number of spheres in a line
static void create_spheres(int num, double x_start, double y_start, double z_start, double x_inc, double y_inc, double z_inc) {
	double x = x_start;
	double y = y_start;
	double z = z_start;
	int i;
	for (i = 0; i < num; i++) {
		spheres[count].id = count;
		spheres[count].pos.x = x;
		spheres[count].pos.y = y;
		spheres[count].pos.z = z;
		spheres[count].vel.x = rand() / (RAND_MAX + 1.0);
		spheres[count].vel.y = rand() / (RAND_MAX + 1.0);
		spheres[count].vel.z = rand() / (RAND_MAX + 1.0);
		spheres[count].mass = 1.0;
		spheres[count].radius = 1.0;
		add_sphere_to_correct_sector(&spheres[count]);
		x += x_inc;
		y += y_inc;
		z += z_inc;
		count++;
	}
}

static void create_sphere(double x, double y, double z, double x_vel, double y_vel, double z_vel) {
	spheres[count].id = count;
	spheres[count].pos.x = x;
	spheres[count].pos.y = y;
	spheres[count].pos.z = z;
	spheres[count].vel.x = x_vel;
	spheres[count].vel.y = y_vel;
	spheres[count].vel.z = z_vel;
	spheres[count].mass = 1.0;
	spheres[count].radius = 1.0;
	add_sphere_to_correct_sector(&spheres[count]);
	count++;
}

// Generates spheres with random velocities;
// Position is hardcoded for now
static void init_spheres() {
	NUM_SPHERES = 2000;
	spheres = calloc(NUM_SPHERES, sizeof(struct sphere_s));
	srand(123);
	create_spheres(250, 10.0, 10.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 40.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 460.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 490.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 510.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 540.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 960.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 990.0, 10.0, 3.5, 0.0, 0.0);
}

// Hardcoded for 4 even sectors at the moment.
static void init_sectors() {
	int count = 0;
	for (int i = 0; i < NUM_SECTORS_X; i++) {
		for (int j = 0; j < NUM_SECTORS_Y; j++) {
			for (int k = 0; k < NUM_SECTORS_Z; k++) {
				struct sector_s *s = &grid->sectors[i][j][0];
				s->start.x = grid->start.x + ((grid->end.x / 2.0) * i);
				s->end.x = grid->start.x + ((grid->end.x / 2.0) * (i + 1));
				s->start.y = grid->start.y + ((grid->end.y / 2.0) * j);
				s->end.y = grid->start.y + ((grid->end.y / 2.0) * (j + 1));
				s->start.z = grid->start.z;
				s->end.z = grid->end.z;
				s->pos.x = i;
				s->pos.y = j;
				s->pos.z = k;
				s->num_spheres = 0;
				s->max_spheres = 2000;
				s->spheres = calloc(s->max_spheres, sizeof(struct sphere_s *));
				count++;
			}
		}
	}
}

// Using hardcoded values for now
void init_grid() {
	grid = calloc(1, sizeof(struct grid_s));
	grid->start.x = 0.0;
	grid->start.y = 0.0;
	grid->start.z = 0.0;
	grid->end.x = 1000.0;
	grid->end.y = 1000.0;
	grid->end.z = 1000.0;
	init_sectors();
	init_spheres();
}

// Finds when all spheres in a given sector will collide with other and returns
// the soonest time.
static void find_collision_times_between_spheres_in_sector(const struct sector_s *sector) {
	int i, j;
	for (i = 0; i < sector->num_spheres - 1; i++) {
		struct sphere_s *s1 = sector->spheres[i];
		for (j = i + 1; j < sector->num_spheres; j++) {
			struct sphere_s *s2 = sector->spheres[j];
			double time = find_collision_time_spheres(s1, s2);
			if (time < event_details.time) {
				event_details.type = COL_TWO_SPHERES;
				event_details.sphere_1 = s1;
				event_details.sphere_2 = s2;
				event_details.time = time;
			}
		}
	}
}

// Finds time to both collide with grid and to cross sector boundaries.
// Can optimise further so that grid boundaries are not checked if another
// sector will be entered first.
static void find_collision_times_grid_boundary_for_sector(const struct sector_s *sector) {
	enum axis axis = COL_NONE;
	int i;
	for (i = 0; i < sector->num_spheres; i++) {
		struct sphere_s *sphere = sector->spheres[i];
		double time = find_collision_time_grid(sphere, &axis);
		if (time < event_details.time) {
			event_details.type = COL_SPHERE_WITH_GRID;
			event_details.sphere_1 = sphere;
			event_details.time = time;
			event_details.grid_axis = axis;
		}
		struct sector_s *temp_dest;
		time = find_collision_time_sector(sector, sphere, &temp_dest);
		if (time < event_details.time) {
			event_details.type = COL_SPHERE_WITH_SECTOR;
			event_details.sphere_1 = sphere;
			event_details.time = time;
			event_details.source_sector = sector;
			event_details.dest_sector = temp_dest;
		}
	}
}

// Given a sector finds the soonest occuring event.
// The event will be either two spheres colliding, a sphere colliding with a grid
// boundary, or a sphere passing into another sector.
// Any partial crossings will be handled once this function has been called for each sector.
static void find_event_times_for_sector(const struct sector_s *sector) {
	if (sector->num_spheres == 0) {
		return;
	}
	find_collision_times_between_spheres_in_sector(sector);
	find_collision_times_grid_boundary_for_sector(sector);
}

// Finds event times for each sector, excluding partial crossings
static void find_event_times_for_all_sectors() {
	for (int x = 0; x < NUM_SECTORS_X; x++) {
		for (int y = 0; y < NUM_SECTORS_Y; y++) {
			for (int z = 0; z < NUM_SECTORS_Z; z++) {
				find_event_times_for_sector(&grid->sectors[x][y][z]);
			}
		}
	}
}

// Given a sphere that is known to be heading towards the given sector
// check if the sphere will collide with spheres in the sector.
static void find_partial_crossing_events_between_sphere_and_sector(const struct sphere_s *sphere_1, const struct sector_s *sector_2) {
	int j;
	for (j = 0; j < sector_2->num_spheres; j++) {
		struct sphere_s *sphere_2 = sector_2->spheres[j];
		double time = find_collision_time_spheres(sphere_1, sphere_2);
		if (time < event_details.time) {
			event_details.type = COL_TWO_SPHERES;
			event_details.sphere_1 = sphere_1;
			event_details.sphere_2 = sphere_2;
			event_details.time = time;
		}
	}
}

// For each sphere check which sectors it is going towards.
// If by the time of the current soonest event it is within a certain distance 
// of any sector it is travelling towards we must check for partial crossings.
// TODO: maybe make this more generic
// TODO: use else if
static void find_partial_crossing_events_for_sector(const struct sector_s *sector) {
	int i;
	for (i = 0; i < sector->num_spheres; i++) {
		struct sphere_s *sphere = sector->spheres[i];
		union vector_3d new_pos;
		new_pos.x = sphere->pos.x + (sphere->vel.x * event_details.time);
		new_pos.y = sphere->pos.y + (sphere->vel.y * event_details.time);
		new_pos.z = sphere->pos.z + (sphere->vel.z * event_details.time);
		enum axis a;
		for (a = X_AXIS; a <= Z_AXIS; a++) { // right/up/forward on x/y/z axis
			if (sphere->vel.vals[a] >= 0.0 && sector->pos.vals[a] != SECTOR_DIMS[a] - 1) {
				int s_x = sector->pos.x + SECTOR_MODIFIERS[DIR_POSITIVE][a][X_AXIS];
				int s_y = sector->pos.y + SECTOR_MODIFIERS[DIR_POSITIVE][a][Y_AXIS];
				int s_z = sector->pos.z + SECTOR_MODIFIERS[DIR_POSITIVE][a][Z_AXIS];
				struct sector_s *sector_2 = &grid->sectors[s_x][s_y][s_z];
				if (new_pos.vals[a] >= sector_2->start.vals[a] - sphere->radius - sector_2->largest_radius) {
					find_partial_crossing_events_between_sphere_and_sector(sphere, sector_2);
				}
			}
		}
		for (a = X_AXIS; a <= Z_AXIS; a++) { // left/down/behind on x/y/z axis
			if (sphere->vel.vals[a] <= 0.0 && sector->pos.vals[a] != 0) {
				int s_x = sector->pos.x + SECTOR_MODIFIERS[DIR_NEGATIVE][a][X_AXIS];
				int s_y = sector->pos.y + SECTOR_MODIFIERS[DIR_NEGATIVE][a][Y_AXIS];
				int s_z = sector->pos.z + SECTOR_MODIFIERS[DIR_NEGATIVE][a][Z_AXIS];
				struct sector_s *sector_2 = &grid->sectors[s_x][s_y][s_z];
				if (new_pos.vals[a] <= sector_2->end.vals[a]  + sphere->radius + sector_2->largest_radius) {
					find_partial_crossing_events_between_sphere_and_sector(sphere, sector_2);
				}
			}
		}
		// right on x axis and up on y axis
		if (sphere->vel.x >= 0.0 && sector->pos.x != SECTOR_DIMS[X_AXIS] - 1 && sphere->vel.y >= 0.0 && sector->pos.y != SECTOR_DIMS[Y_AXIS] - 1) {
			struct sector_s *sector_2 = &grid->sectors[sector->pos.x + 1][sector->pos.y + 1][sector->pos.z];
			if (new_pos.x >= sector_2->start.x - sphere->radius - sector_2->largest_radius && new_pos.y >= sector_2->start.y - sphere->radius - sector_2->largest_radius) {
				find_partial_crossing_events_between_sphere_and_sector(sphere, sector_2);
			}
		}
		// left on x axis and up on y axis
		if (sphere->vel.x <= 0.0 && sector->pos.x != 0 && sphere->vel.y >= 0.0 && sector->pos.y != SECTOR_DIMS[Y_AXIS] - 1) {
			struct sector_s *sector_2 = &grid->sectors[sector->pos.x - 1][sector->pos.y + 1][sector->pos.z];
			if (new_pos.x <= sector_2->end.x + sphere->radius + sector_2->largest_radius && new_pos.y >= sector_2->start.y - sphere->radius - sector_2->largest_radius) {
				find_partial_crossing_events_between_sphere_and_sector(sphere, sector_2);
			}
		}
		// right on x axis and down on y axis
		if (sphere->vel.x >= 0.0 && sector->pos.x != SECTOR_DIMS[X_AXIS] - 1 && sphere->vel.y <= 0.0 && sector->pos.y != 0) {
			struct sector_s *sector_2 = &grid->sectors[sector->pos.x + 1][sector->pos.y - 1][sector->pos.z];
			if (new_pos.x >= sector_2->start.x - sphere->radius - sector_2->largest_radius && new_pos.y <= sector_2->end.y + sphere->radius + sector_2->largest_radius) {
				find_partial_crossing_events_between_sphere_and_sector(sphere, sector_2);
			}
		}
		// left on x axis and down on y axis
		if (sphere->vel.x <= 0.0 && sector->pos.x != 0 && sphere->vel.y <= 0.0 && sector->pos.y != 0) {
			struct sector_s *sector_2 = &grid->sectors[sector->pos.x - 1][sector->pos.y - 1][sector->pos.z];
			if (new_pos.x <= sector_2->end.x + sphere->radius + sector_2->largest_radius && new_pos.y <= sector_2->end.y + sphere->radius + sector_2->largest_radius) {
				find_partial_crossing_events_between_sphere_and_sector(sphere, sector_2);
			}
		}
		// TODO y/z
		// TODO: 3d diagonal
	}
}

// If needed find any sphere on sphere collisions that occur between spheres
// that partially cross sector boundaries. 
static void find_partial_crossing_events_for_all_sectors() {
	for (int x = 0; x < NUM_SECTORS_X; x++) {
		for (int y = 0; y < NUM_SECTORS_Y; y++) {
			for (int z = 0; z < NUM_SECTORS_Z; z++) {
				find_partial_crossing_events_for_sector(&grid->sectors[x][y][z]);
			}
		}
	}
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
	} else if (event_details.type == COL_TWO_SPHERES) {
		apply_bounce_between_spheres(event_details.sphere_1, event_details.sphere_2);
	} else if (event_details.type == COL_SPHERE_WITH_SECTOR) {
		remove_sphere_from_sector(event_details.source_sector, event_details.sphere_1);
		add_sphere_to_sector(event_details.dest_sector, event_details.sphere_1);
	}
}

// For debugging
// Ensures each sphere is located within the sector responsible for it.
// Helps catch any issues with transfering spheres between sectors.
static void sanity_check() {
	for (int x = 0; x < NUM_SECTORS_X; x++) {
		for (int y = 0; y < NUM_SECTORS_Y; y++) {
			for (int z = 0; z < NUM_SECTORS_Z; z++) {
				struct sector_s *s = &grid->sectors[x][y][z];
				int i;
				for (i = 0; i < s->num_spheres; i++) {
					struct sphere_s *sphere = s->spheres[i];
					int error = 0;
					enum axis a;
					for (a = X_AXIS; a <= Z_AXIS; a++) {
						if (sphere->pos.vals[a] > s->end.vals[a]) {
							error = 1;
						}
						if (sphere->pos.vals[a] < s->start.vals[a]) {
							error = 1;
						}
					}
					if (error) {
						printf("Sphere not in correct sector!\n");
						getchar();
						exit(1);
					}
				}
			}
		}
	}
}

// TODO: highly work in progress
double update_grid(double limit, double time_elapsed) {
	sanity_check();
	// First reset records.
	event_details.time = DBL_MAX;
	event_details.sphere_1 = NULL;
	event_details.sphere_2 = NULL;
	event_details.source_sector = NULL;
	event_details.dest_sector = NULL;
	event_details.type = COL_NONE;
	event_details.grid_axis = AXIS_NONE;
	// Now find event + time of event
	find_event_times_for_all_sectors();
	find_partial_crossing_events_for_all_sectors();
	// Final event may take place after time limit, so cut it short
	if (limit - time_elapsed < event_details.time) {
		event_details.time = limit - time_elapsed;
		event_details.type = COL_NONE;
	}
	// Lastly move forward to the next event
	update_spheres();
	sanity_check();
	return event_details.time;
}
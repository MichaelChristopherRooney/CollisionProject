#include <float.h>
#include <stdlib.h>
#include <stdio.h>

#include "collision.h"
#include "grid.h"

// Hardcoded for testing right now
static void init_spheres() {
	NUM_SPHERES = 2;
	spheres = calloc(NUM_SPHERES, sizeof(struct sphere_s));
	int count = 0;
	spheres[count].id = count;
	spheres[count].pos.x = 10.0;
	spheres[count].pos.y = 10.0;
	spheres[count].pos.z = 10.0;
	spheres[count].vel.x = 30.0;
	spheres[count].vel.y = 0.0;
	spheres[count].vel.z = 0.0;
	spheres[count].mass = 1.0;
	spheres[count].radius = 1.0;
	add_sphere_to_sector(&grid->sectors[0][0][0], &spheres[count]);
	count = 1;
	spheres[count].id = count;
	spheres[count].pos.x = 90.0;
	spheres[count].pos.y = 10.0;
	spheres[count].pos.z = 10.0;
	spheres[count].vel.x = -30.0;
	spheres[count].vel.y = 0.0;
	spheres[count].vel.z = 0.0;
	spheres[count].mass = 1.0;
	spheres[count].radius = 1.0;
	add_sphere_to_sector(&grid->sectors[1][0][0], &spheres[count]);
	/*count = 1;
	spheres[count].pos.x = 20;
	spheres[count].pos.y = 10.0;
	spheres[count].pos.z = 10.0;
	spheres[count].vel.x = 20.0;
	spheres[count].vel.y = 0.0;
	spheres[count].vel.z = 0.0;
	spheres[count].mass = 1.0;
	spheres[count].radius = 1.0;
	add_sphere_to_sector(&grid->sectors[0][0][0], &spheres[count]);
	/*
	NUM_SPHERES = 4;
	spheres = calloc(NUM_SPHERES, sizeof(struct sphere_s));
	int count = 0;
	for (int x = 0; x < 2; x++) {
		for (int y = 0; y < 2; y++) {
			spheres[count].pos.x = (250.0 * x) + 125.0;
			spheres[count].pos.y = (250.0 * y) + 125.0;
			spheres[count].pos.z = 10.0;
			spheres[count].vel.x = 1.0;
			spheres[count].vel.y = 1.0;
			spheres[count].vel.z = 1.0;
			spheres[count].mass = 1.0;
			spheres[count].radius = 1.0;
			grid->sectors[x][y][0].head = calloc(1, sizeof(struct sphere_list_s));
			grid->sectors[x][y][0].head->sphere = &spheres[count];
			count++;
		}
	}*/
}

// Hardcoded for 4 even sectors at the moment.
static void init_sectors() {
	int count = 0;
	for (int i = 0; i < NUM_SECTORS_X; i++) {
		for (int j = 0; j < NUM_SECTORS_Y; j++) {
			for (int k = 0; k < NUM_SECTORS_Z; k++) {
				grid->sectors[i][j][0].start.x = grid->start.x + ((grid->end.x / 2.0) * i);
				grid->sectors[i][j][0].end.x = grid->start.x + ((grid->end.x / 2.0) * (i + 1));
				grid->sectors[i][j][0].start.y = grid->start.y + ((grid->end.y / 2.0) * j);
				grid->sectors[i][j][0].end.y = grid->start.y + ((grid->end.y / 2.0) * (j + 1));
				grid->sectors[i][j][0].start.z = grid->start.z;
				grid->sectors[i][j][0].end.z = grid->end.z;
				grid->sectors[i][j][0].pos.x = i;
				grid->sectors[i][j][0].pos.y = j;
				grid->sectors[i][j][0].pos.z = k;
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
	grid->end.x = 100.0;
	grid->end.y = 100.0;
	grid->end.z = 100.0;
	init_sectors();
	init_spheres();
}

// Finds when all spheres in a given sector will collide with other and returns
// the soonest time.
static void find_collision_times_between_spheres_in_sector(const struct sector_s *sector) {
	struct sphere_list_s *cur = sector->head;
	while (cur != NULL) {
		struct sphere_s *s1 = cur->sphere;
		struct sphere_list_s *next = cur->next;
		while (next != NULL) {
			struct sphere_s *s2 = next->sphere;
			double time = find_collision_time_spheres(s1, s2);
			if (time < event_details.time) {
				event_details.type = COL_TWO_SPHERES;
				event_details.sphere_1 = s1;
				event_details.sphere_2 = s2;
				event_details.time = time;
			}
			next = next->next;
		}
		cur = cur->next;
	}
}

// Finds time to both collide with grid and to cross sector boundaries.
// Can optimise further so that grid boundaries are not checked if another
// sector will be entered first.
static void find_collision_times_grid_boundary_for_sector(const struct sector_s *sector) {
	enum axis axis = COL_NONE;
	struct sphere_list_s *cur = sector->head;
	while (cur != NULL) {
		struct sphere_s *sphere = cur->sphere;
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
		cur = cur->next;
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
	struct sphere_list_s *cur_2 = sector_2->head;
	while (cur_2 != NULL) {
		struct sphere_s *sphere_2 = cur_2->sphere;
		double time = find_collision_time_spheres(sphere_1, sphere_2);
		if (time < event_details.time) {
			event_details.type = COL_TWO_SPHERES;
			event_details.sphere_1 = sphere_1;
			event_details.sphere_2 = sphere_2;
			event_details.time = time;
		}
		cur_2 = cur_2->next;
	}
}

// For each sphere in sector_1 check if it is within its radius + sector_2->largest_radius
// of sector_2 at soonest_time.
// If so then we need to check its collision against spheres in sector_2.
// If sector_2 is further along the axis than sector_1 dir will be 1, else -1.
static void find_partial_crossing_events_between_sectors_non_diagonal(const struct sector_s *sector_1, const struct sector_s *sector_2, const enum coord c, const int dir) {
	if (sector_2->num_spheres == 0) {
		return;
	}
	struct sphere_list_s *cur_1 = sector_1->head;
	while (cur_1 != NULL) {
		struct sphere_s *sphere_1 = cur_1->sphere;
		if (dir == 1 && sphere_1->vel.vals[c] > 0.0) {
			double pos_new = sphere_1->pos.vals[c] + (sphere_1->vel.vals[c] * event_details.time);
			if (pos_new >= sector_2->start.vals[c] - sphere_1->radius - sector_2->largest_radius) {
				find_partial_crossing_events_between_sphere_and_sector(sphere_1, sector_2);
			}
		} else if (dir == -1 && sphere_1->vel.vals[c] < 0.0) {
			double pos_new = sphere_1->pos.vals[c] + (sphere_1->vel.vals[c] * event_details.time);
			if (pos_new <= sector_2->end.vals[c] + sphere_1->radius + sector_2->largest_radius) {
				find_partial_crossing_events_between_sphere_and_sector(sphere_1, sector_2);
			}
		}
		cur_1 = cur_1->next;
	}

}

// Iterates over every adjacent sector in each axis and looks for partial crossings.
// TODO: diagonally adjacent sectors
static void find_partial_crossing_events_for_sector(const struct sector_s *sector) {
	if (sector->num_spheres == 0) {
		return;
	}
	enum coord c;
	for (c = X_COORD; c <= Z_COORD; c++) {
		if (sector->pos.vals[c] != 0) { // behind/below/left
			struct sector_s *sector_2 = get_sector_in_negative_direction(sector, c);
			find_partial_crossing_events_between_sectors_non_diagonal(sector, sector_2, c, -1);
		}
		if (sector->pos.vals[c] != SECTOR_DIMS[c] - 1) { // front/above/right
			struct sector_s *sector_2 = get_sector_in_positive_direction(sector, c);
			find_partial_crossing_events_between_sectors_non_diagonal(sector, sector_2, c, 1);
		}
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
				struct sphere_list_s *cur = s->head;
				while (cur != NULL) {
					struct sphere_s *sphere = cur->sphere;
					int error = 0;
					enum coord c;
					for (c = X_COORD; c <= Z_COORD; c++) {
						if (sphere->pos.vals[c] > s->end.vals[c]) {
							error = 1;
						}
						if (sphere->pos.vals[c] < s->start.vals[c]) {
							error = 1;
						}
					}
					if (error) {
						printf("Sphere not in correct sector!\n");
						getchar();
						exit(1);
					}
					cur = cur->next;
				}
			}
		}
	}
}

// TODO: highly work in progress
double update_grid() {
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
	// Lastly move forward to the next event
	update_spheres();
	sanity_check();
	return event_details.time;
}
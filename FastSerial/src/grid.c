#include <float.h>
#include <stdlib.h>

#include "collision.h"
#include "grid.h"

// Hardcoded for testing right now
static void init_spheres() {
	NUM_SPHERES = 1;
	spheres = calloc(NUM_SPHERES, sizeof(struct sphere_s));
	int count = 0;
	spheres[count].pos.x = 10.0;
	spheres[count].pos.y = 10.0;
	spheres[count].pos.z = 10.0;
	spheres[count].vel.x = 10.0;
	spheres[count].vel.y = 0.0;
	spheres[count].vel.z = 0.0;
	spheres[count].mass = 1.0;
	spheres[count].radius = 1.0;
	add_sphere_to_sector(&grid->sectors[0][0][0], &spheres[count]);
	count = 1;
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
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			grid->sectors[i][j][0].x_start = grid->x_start + ((grid->x_end / 2.0) * i);
			grid->sectors[i][j][0].x_end = grid->x_start + ((grid->x_end / 2.0) * (i+1));
			grid->sectors[i][j][0].x_start = grid->y_start + ((grid->y_end / 2.0) * j);
			grid->sectors[i][j][0].x_end = grid->y_start + ((grid->y_end / 2.0) * (j + 1));
			grid->sectors[i][j][0].z_start = grid->z_start;
			grid->sectors[i][j][0].z_end = grid->z_end;
			count++;
		}
	}
}

// Using hardcoded values for now
void init_grid() {
	grid = calloc(1, sizeof(struct grid_s));
	grid->x_start = 0.0;
	grid->y_start = 0.0;
	grid->z_start = 0.0;
	grid->x_end = 500.0;
	grid->y_end = 500.0;
	grid->z_end = 500.0;
	init_sectors();
	init_spheres();
}

// Finds when all spheres in a given sector will collide with other and returns
// the soonest time.
static double find_collision_times_between_spheres_in_sector(struct sector_s *sector) {
	double soonest_time = DBL_MAX; 
	if (sector->num_spheres == 1) {
		return soonest_time;
	}
	struct sphere_list_s *cur = sector->head;
	do {
		struct sphere_s *s1 = cur->sphere;
		struct sphere_list_s *next = cur->next;
		while (next != NULL) {
			struct sphere_s *s2 = next->sphere;
			double time = find_collision_time_spheres(s1, s2);
			if (time < soonest_time) {
				soonest_time = time;
			}
			next = next->next;
		}
		cur = cur->next;
	} while (cur != NULL);
	return soonest_time;
}

// Finds time to both collide with grid and to cross sector boundaries.
// Can optimise further so that grid boundaries are not checked if another
// sector will be entered first.
// TODO: sector boundaries
static double find_collision_times_grid_boundary_for_sector(struct sector_s *sector) {
	double soonest_time = DBL_MAX;
	enum axis axis = COL_NONE;
	struct sphere_list_s *cur = sector->head;
	do {
		struct sphere_s *sphere = cur->sphere;
		double time = find_collision_time_grid(sphere, &axis);
		if (time < soonest_time) {
			soonest_time = time;
		}
		cur = cur->next;
	} while (cur != NULL);
	return soonest_time;
}

// Given a sector finds the soonest occuring event.
// The event will be either two spheres colliding, a sphere colliding with a grid
// boundary, or a sphere passing into another sector.
// TODO: handle transfering spheres between sectors.
// TODO: handle edge case where spheres are partially over sector line.
static double find_event_times_for_sector(struct sector_s *sector, int x, int y, int z) {
	double soonest_time = DBL_MAX;
	if (sector->num_spheres == 0) {
		return soonest_time;
	}
	soonest_time = find_collision_times_between_spheres_in_sector(sector);
	double time = find_collision_times_grid_boundary_for_sector(sector);
}

// TODO: highly work in progress
double update_grid() {
	double soonest_time = DBL_MAX;
	for (int x = 0; x < NUM_SECTORS_X; x++) {
		for (int y = 0; y < NUM_SECTORS_Y; y++) {
			for (int z = 0; z < NUM_SECTORS_Z; z++) {
				double time = find_event_times_for_sector(&grid->sectors[x][y][z], x, y, z);
				if (time < soonest_time) {
					soonest_time = time;
				}
			}
		}
	}
	return 0.0;
}
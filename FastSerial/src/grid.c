#include <float.h>
#include <stdlib.h>

#include "collision.h"
#include "grid.h"

// Hardcoded for testing right now
static void init_spheres() {
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
	}
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

// Given a sector finds the soonest occuring event.
// The event will be either two spheres colliding, a sphere colliding with a grid
// boundary, or a sphere passing into another sector.
// TODO: handle transfering spheres between sectors.
// TODO: handle edge case where spheres are partially over sector line.
static void find_event_times_for_sector(struct sector_s *sector, int x, int y, int z) {
	if (grid->sectors[x][y][z].head == NULL) {
		return;
	}
	struct sphere_list_s *cur = grid->sectors[x][y][z].head;
	do {
		struct sphere_s *s = cur->sphere;
		cur = cur->next;
	} while (cur != NULL);
}

// TODO: highly work in progress
double update_grid() {
	for (int x = 0; x < NUM_SECTORS_X; x++) {
		for (int y = 0; y < NUM_SECTORS_Y; y++) {
			for (int z = 0; z < NUM_SECTORS_Z; z++) {
				find_event_times_for_sector(&grid->sectors[x][y][z], x, y, z);
			}
		}
	}
}
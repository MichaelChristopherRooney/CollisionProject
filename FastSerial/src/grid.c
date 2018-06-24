#include <float.h>
#include <stdlib.h>

#include "collision.h"
#include "grid.h"

static double soonest_time; // When the event happens
static enum collision_type event; // What the next event is.
static struct sphere_s *event_sphere_1; // Sphere that hits the grid, transfers to another sector, or is the first sphere in a sphere on sphere collision.
static struct sphere_s *event_sphere_2; // Second sphere in a sphere on sphere collision, otherwise NULL
static enum axis event_axis; // If the event is a sphere bouncing off the grid record the axis
// If a sphere moves between sectors these record the details.
static struct sector_s *source_sector;
static struct sector_s *dest_sector;

// Hardcoded for testing right now
static void init_spheres() {
	NUM_SPHERES = 1;
	spheres = calloc(NUM_SPHERES, sizeof(struct sphere_s));
	int count = 0;
	spheres[count].pos.x = 10.0;
	spheres[count].pos.y = 10.0;
	spheres[count].pos.z = 10.0;
	spheres[count].vel.x = 200.0;
	spheres[count].vel.y = 0.0;
	spheres[count].vel.z = 0.0;
	spheres[count].mass = 1.0;
	spheres[count].radius = 1.0;
	add_sphere_to_sector(&grid->sectors[0][0][0], &spheres[count]);
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
				grid->sectors[i][j][0].x_start = grid->x_start + ((grid->x_end / 2.0) * i);
				grid->sectors[i][j][0].x_end = grid->x_start + ((grid->x_end / 2.0) * (i + 1));
				grid->sectors[i][j][0].y_start = grid->y_start + ((grid->y_end / 2.0) * j);
				grid->sectors[i][j][0].y_end = grid->y_start + ((grid->y_end / 2.0) * (j + 1));
				grid->sectors[i][j][0].z_start = grid->z_start;
				grid->sectors[i][j][0].z_end = grid->z_end;
				grid->sectors[i][j][0].x = i;
				grid->sectors[i][j][0].y = j;
				grid->sectors[i][j][0].z = k;
				count++;
			}
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
static void find_collision_times_between_spheres_in_sector(const struct sector_s *sector) {
	struct sphere_list_s *cur = sector->head;
	while (cur != NULL) {
		struct sphere_s *s1 = cur->sphere;
		struct sphere_list_s *next = cur->next;
		while (next != NULL) {
			struct sphere_s *s2 = next->sphere;
			double time = find_collision_time_spheres(s1, s2);
			if (time < soonest_time) {
				event = COL_TWO_SPHERES;
				event_sphere_1 = s1;
				event_sphere_2 = s2;
				soonest_time = time;
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
		if (time < soonest_time) {
			event = COL_SPHERE_WITH_GRID;
			event_sphere_1 = sphere;
			soonest_time = time;
		}
		struct sector_s *temp_dest;
		time = find_collision_time_sector(sector, sphere, &temp_dest);
		if (time < soonest_time) {
			event = COL_SPHERE_WITH_SECTOR;
			event_sphere_1 = sphere;
			soonest_time = time;
			source_sector = sector;
			dest_sector = temp_dest;
		}
		cur = cur->next;
	}
}

// Given a sector finds the soonest occuring event.
// The event will be either two spheres colliding, a sphere colliding with a grid
// boundary, or a sphere passing into another sector.
// TODO: handle edge case where spheres are partially over sector line.
static void find_event_times_for_sector(struct sector_s *sector, int x, int y, int z) {
	if (sector->num_spheres == 0) {
		return;
	}
	find_collision_times_between_spheres_in_sector(sector);
	find_collision_times_grid_boundary_for_sector(sector);
}

// TODO: highly work in progress
double update_grid() {
	// reset
	soonest_time = DBL_MAX;
	event_sphere_1 = NULL;
	event_sphere_2 = NULL;
	source_sector = NULL;
	dest_sector = NULL;
	event = COL_NONE;
	// TODO: first thing to be done is check have any spheres moved outside their sector boundaries
	for (int x = 0; x < NUM_SECTORS_X; x++) {
		for (int y = 0; y < NUM_SECTORS_Y; y++) {
			for (int z = 0; z < NUM_SECTORS_Z; z++) {
				find_event_times_for_sector(&grid->sectors[x][y][z], x, y, z);
			}
		}
	}
	int i;
	for (i = 0; i < NUM_SPHERES; i++) {
		struct sphere_s *s = &(spheres[i]);
		update_sphere_position(s, soonest_time);
	}
	if (event == COL_SPHERE_WITH_GRID) {
		switch (event_axis) {
		case X_AXIS:
			event_sphere_1->vel.x *= -1;
			break;
		case Y_AXIS:
			event_sphere_1->vel.y *= -1;
			break;
		case Z_AXIS:
			event_sphere_1->vel.z *= -1;
			break;
		}
	} else if (event == COL_TWO_SPHERES) {
		apply_bounce_between_spheres(event_sphere_1, event_sphere_2);
	} else if (event == COL_SPHERE_WITH_SECTOR) {
		remove_sphere_from_sector(source_sector, event_sphere_1);
		add_sphere_to_sector(dest_sector, event_sphere_1);
	}
	return soonest_time;
}
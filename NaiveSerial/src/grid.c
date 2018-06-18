#include <float.h>
#include <stdlib.h>

#include "collision.h"
#include "grid.h"

/*
TODO figure out why this breaks right away
probably because time to next event is 0 as they are already touching
NUM_SPHERES = 1000;
spheres = calloc(NUM_SPHERES, sizeof(struct sphere_s));
double x = 2.0;
double y = 2.0;
int i, j;
int count = 0;
for (i = 0; i < 10; i++) {
for (j = 0; j < 100; j++) {
spheres[count].pos.x = x;
spheres[count].pos.y = y;
spheres[count].pos.z = 10.0;
spheres[count].vel.x = rand() / (RAND_MAX + 1.0);
spheres[count].vel.y = rand() / (RAND_MAX + 1.0);
spheres[count].vel.z = rand() / (RAND_MAX + 1.0);
spheres[count].mass = 1.0;
spheres[count].radius = 1.0;
y = y + 2.0;
count++;
}
x = x + 2.0;
}
*/

// Generates spheres with random velocities;
// Position is hardcoded for now
static void init_spheres() {
	NUM_SPHERES = 1000;
	spheres = calloc(NUM_SPHERES, sizeof(struct sphere_s));
	double x = 2.0;
	double y = 2.0;
	int i, j;
	int count = 0;
	for (i = 0; i < 10; i++) {
		for (j = 0; j < 100; j++) {
			spheres[count].pos.x = x;
			spheres[count].pos.y = y;
			spheres[count].pos.z = 10.0;
			spheres[count].vel.x = rand() / (RAND_MAX + 1.0);
			spheres[count].vel.y = rand() / (RAND_MAX + 1.0);
			spheres[count].vel.z = rand() / (RAND_MAX + 1.0);
			spheres[count].mass = 1.0;
			spheres[count].radius = 1.0;
			y = y + 4.0;
			count++;
		}
		y = 2.0;
		x = x + 4.0;
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
	init_spheres();
}

// For each sphere finds when it will collide with the grid, then finds when it
// will collide with all other spheres.
// The time to the soonest collision is found and all spheres are advanced by 
// this time step.
// Then the colliding sphere(s) have their velocity updated, or if the soonest
// collision is a sphere colliding with a grid then its velocity on the collision
// axis is simply inverted.
//
// Variables used:
// "soonest_time": Time to the soonest occuring collision.
// "type": Type of the soonest occuring collision - either sphere on sphere or sphere on grid
// "col_sphere_1": First sphere in the soonest occuring collision.
// "col_sphere_2": Second sphere, NULL if first sphere will collide with grid
// "col_axis": If soonest collision is sphere on grid this stores the axis it occurs on.
double update_grid() {
	double soonest_time = DBL_MAX; 
	enum collision_type col_type = COL_NONE; 
	enum AXIS col_axis = AXIS_NONE;
	struct sphere_s *col_sphere_1 = NULL;
	struct sphere_s *col_sphere_2 = NULL;
	int i, j;
	for (i = 0; i < NUM_SPHERES; i++) {
		struct sphere_s *s1 = &(spheres[i]);
		enum AXIS axis;
		double time = find_collision_time_grid(s1, &axis);
		if (time < soonest_time) {
			col_type = COL_SPHERE_WITH_GRID;
			col_axis = axis;
			soonest_time = time;
			col_sphere_1 = s1;
			col_sphere_2 = NULL;
		}
		for (j = i + 1; j < NUM_SPHERES; j++) {
			struct sphere_s *s2 = &(spheres[j]);
			time = find_collision_time_spheres(s1, s2);
			if (time < soonest_time) {
				col_type = COL_TWO_SPHERES;
				col_axis = AXIS_NONE;
				soonest_time = time;
				col_sphere_1 = s1;
				col_sphere_2 = s2;
			}
		}
	}
	for (i = 0; i < NUM_SPHERES; i++) {
		struct sphere_s *s = &(spheres[i]);
		update_sphere_position(s, soonest_time);
	}
	if (col_type == COL_SPHERE_WITH_GRID) {
		switch (col_axis) {
		case X_AXIS:
			col_sphere_1->vel.x *= -1;
			break;
		case Y_AXIS:
			col_sphere_1->vel.y *= -1;
			break;
		case Z_AXIS:
			col_sphere_1->vel.z *= -1;
			break;
		}
	} else if (col_type == COL_TWO_SPHERES) {
		apply_bounce_between_spheres(col_sphere_1, col_sphere_2);
	}
	return soonest_time;
}
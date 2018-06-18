#include <float.h>
#include <stdlib.h>

#include "collision.h"
#include "grid.h"


struct sphere_s spheres[NUM_SPHERES] = {
{ { -1.0, 0.0, 0.0 },{ 20.0, 10.0, 10.0 }, 1.0, 1.0 },
{ { 0.0, 0.0, 0.0 },{ 10.0, 10.0, 10.0 }, 1.0, 1.0 },

{ { 1.0, -1.0, 0.0 },{ 10.0, 10.0, 20.0 }, 1.0, 1.0 },
{ { 1.0, 0.0,-1.0 },{ 10.0, 30.0, 10.0 }, 1.0, 1.0 },
{ { -1.0, -2.0, 0.0 },{ 20.0, 10.0, 40.0 }, 1.0, 1.0 },

};


/*
struct sphere_s spheres[NUM_SPHERES] = {
	{ { 1.0, 0.1, 0.01 },{ 10.0, 10.0, 10.0 }, 1.0, 1.0 },
	{ { 2.0, 0.2, 0.02 },{ 20.0, 20.0, 20.0 }, 1.0, 1.0 },
	{ { 3.0, 0.3, 0.03 },{ 30.0, 30.0, 30.0 }, 1.0, 1.0 },
	{ { 4.0, 0.4, 0.04 },{ 40.0, 40.0, 40.0 }, 1.0, 1.0 },
	{ { 5.0, 0.5, 0.05 },{ 45.0, 45.0, 45.0 }, 1.0, 1.0 },

};*/

// Using hardcoded values for now
void init_grid() {
	grid = calloc(1, sizeof(struct grid_s));
	grid->x_start = 0.0;
	grid->y_start = 0.0;
	grid->z_start = 0.0;
	grid->x_end = 50.0;
	grid->y_end = 50.0;
	grid->z_end = 50.0;
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
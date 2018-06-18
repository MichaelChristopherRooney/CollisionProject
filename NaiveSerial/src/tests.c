#include <float.h>
#include <stdio.h>
#include <stdlib.h>

#include "collision.h"
#include "grid.h"
#include "sphere.h"
#include "vector_3.h"

// A simple case where the first sphere is stationary at (10, 10, 10) and the second
// sphere moves down along the x-axis starting at (20, 10, 10) and both spheres
// have the same radius and mass.
// The two spheres should collide after 8 seconds.
// In addition if the collision were not occur the first sphere will not 
// collide with the grid, and the second sphere should collide with the grid
// after 19 seconds.
// Once the collision has happened the first sphere should absorb all of the
// second sphere's momentum/energy.
static void test_1(char *test_name) {
	struct sphere_s s2 = { { -1.0, 0.0, 0.0 }, { 20.0, 10.0, 10.0 }, 1.0, 1.0 };
	struct sphere_s s1 = { { 0.0, 0.0, 0.0 }, { 10.0, 10.0, 10.0 }, 1.0, 1.0 };
	double t = find_collision_time_spheres(&s1, &s2);
	if (t != 8.0) {
		printf("%s: FAILED. Time of collision should be 8.0 but is %f\n", test_name, t);
		return;
	}
	enum axis col_axis;
	double gt = find_collision_time_grid(&s1, &col_axis);
	if (col_axis != AXIS_NONE) {
		printf("%s: FAILED. First sphere should NOT collide with grid, but result says it will.\n", test_name);
		return;
	}
	gt = find_collision_time_grid(&s2, &col_axis);
	if (col_axis != X_AXIS) {
		printf("%s: FAILED. Second sphere should collide with grid on x-axis, but result says it will not.\n", test_name);
		return;
	}
	if (gt != 19.0) {
		printf("%s: FAILED. Sphere two should collide with grid after 19.0 seconds, but got %f seconds.\n", test_name, gt);
		return;
	}
	double momentum_before = (s1.vel.x * s1.mass) + (s2.vel.x * s2.mass);
	double energy_before = (0.5 * (s1.vel.x * s1.vel.x) * s1.mass) + (0.5 * (s2.vel.x * s2.vel.x) * s2.mass);
	update_sphere_position(&s1, t);
	update_sphere_position(&s2, t);
	apply_bounce_between_spheres(&s1, &s2);
	if (s1.vel.x != -1.0) {
		printf("%s: FAILED. After collision sphere one should have x velocity of -1.0 but instead has %f\n", test_name, s1.vel.x);
		return;
	}
	if (s2.vel.x != 0.0) {
		printf("%s: FAILED. After collision sphere one should have x velocity of 0.0 but instead has %f\n", test_name, s2.vel.x);
		return;
	}
	double momentum_after = (s1.vel.x * s1.mass) + (s2.vel.x * s2.mass);
	double energy_after = (0.5 * (s1.vel.x * s1.vel.x) * s1.mass) + (0.5 * (s2.vel.x * s2.vel.x) * s2.mass);
	if (momentum_before != momentum_after) {
		printf("%s: FAILED. After collision momentum was not conserved\n", test_name);
		return;
	}
	if (momentum_before != momentum_after) {
		printf("%s: FAILED. After collision energy was not conserved\n", test_name);
		return;
	}
	printf("%s: PASSED.\n", test_name);
}

void run_tests() {
	test_1("Test one");
}
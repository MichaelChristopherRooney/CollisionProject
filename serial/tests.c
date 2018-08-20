#include <float.h>
#include <stdio.h>
#include <stdlib.h>

#include "collision.h"
#include "grid.h"
#include "simulation.h"
#include "sphere.h"
#include "vector_3.h"

struct test_data_s {
	struct sphere_s *s1;
	struct sphere_s *s2;
	double col_time; // when spheres should impact (DBL_MAX if no impact)
	double s1_grid_time; // when sphere 1 impacts grid (DBL_MAX if no impact)
	double s2_grid_time; // when sphere 2 impacts grid (DBL_MAX if no impact)
	enum axis s1_grid_axis; // grid axis sphere 1 impacts on (AXIS_NONE if no impact)
	enum axis s2_grid_axis; // grid axis sphere 2 impacts on (AXIS_NONE if no impact)
	union vector_3d s1_vel_after; // velocity of sphere 1 after impact
	union vector_3d s2_vel_after; // velocity of sphere 2 after impact
	char *test_name;
};

static bool check_grid_collisions(struct test_data_s *data){
	enum axis col_axis;
	double gt = find_collision_time_grid(data->s1, &col_axis);
	if(data->s1_grid_axis != col_axis || gt != data->s1_grid_time){
		printf("%s: FAILED. Sphere one will not impact with grid as expected.\n", data->test_name);
		return false;
	}
	gt = find_collision_time_grid(data->s2, &col_axis);
	if(data->s2_grid_axis != col_axis || gt != data->s2_grid_time){
		printf("%s: FAILED. Sphere two will not impact with grid as expected.\n", data->test_name);
		return false;
	}	
	return true;
}

static void apply_event_for_test(struct test_data_s *data){
	if(data->col_time < data->s1_grid_time && data->col_time < data->s1_grid_time){
		update_sphere_position(data->s1, data->col_time);
		update_sphere_position(data->s2, data->col_time);
		apply_bounce_between_spheres(data->s1, data->s2);
	} else if(data->s1_grid_time < data->s2_grid_time){

	} else {

	}
}

static void test_harness(struct test_data_s *data){
	double t = find_collision_time_spheres(data->s1, data->s2);
	if (t != data->col_time) {
		printf("%s: FAILED. Time of collision should be %f but is %f\n", data->test_name, data->col_time, t);
		return;
	}
	if(!check_grid_collisions(data)){
		return;
	}
	double momentum_before = (data->s1->vel.x * data->s1->mass) + (data->s2->vel.x * data->s2->mass);
	double energy_before = (0.5 * (data->s1->vel.x * data->s1->vel.x) * data->s1->mass) + (0.5 * (data->s2->vel.x * data->s2->vel.x) * data->s2->mass);
	apply_event_for_test(data);
	if (data->s1->vel.x != data->s1_vel_after.x || data->s1->vel.y != data->s1_vel_after.y || data->s1->vel.z != data->s1_vel_after.z) {
		printf("%s: FAILED. Sphere one has incorrect velocity after collision\n", data->test_name);
		return;
	}
	if (data->s2->vel.x != data->s2_vel_after.x || data->s2->vel.y != data->s2_vel_after.y || data->s2->vel.z != data->s2_vel_after.z) {
		printf("%s: FAILED. Sphere two has incorrect velocity after collision\n", data->test_name);
		return;
	}
	double momentum_after = (data->s1->vel.x * data->s1->mass) + (data->s2->vel.x * data->s2->mass);
	double energy_after = (0.5 * (data->s1->vel.x * data->s1->vel.x) * data->s1->mass) + (0.5 * (data->s2->vel.x * data->s2->vel.x) * data->s2->mass);
	if (momentum_before != momentum_after) {
		printf("%s: FAILED. After collision momentum was not conserved\n", data->test_name);
		return;
	}
	if (energy_before != energy_after) {
		printf("%s: FAILED. After collision energy was not conserved\n", data->test_name);
		return;
	}
	printf("%s: PASSED.\n", data->test_name);
}

// A very simple test
// Sphere 1 is stationary and sphere 2 is moving towards it along the x axis.
// Sphere 2 hits sphere 1 after 8.0 and transfers all of its energy to it.
static void test_1() {
	// Set up first sphere
	struct sphere_s s1;
	s1.vel.x = 0.0; s1.vel.y = 0.0; s1.vel.z = 0.0;
	s1.pos.x = 10.0; s1.pos.y = 10.0; s1.pos.z = 10.0;
	s1.mass = 1.0; s1.radius = 1.0;
	// Set up second sphere
	struct sphere_s s2;
	s2.vel.x = -1.0; s2.vel.y = 0.0; s2.vel.z = 0.0;
	s2.pos.x = 20.0; s2.pos.y = 10.0; s2.pos.z = 10.0;
	s2.mass = 1.0; s2.radius = 1.0;
	// Set up grid boundaries
	sim_data.grid_size.x = 100.0;
	sim_data.grid_size.y = 100.0;
	sim_data.grid_size.z = 100.0;
	// Set up expected results
	struct test_data_s data;
	data.s1 = &s1;
	data.s2 = &s2;
	data.col_time = 8.0;
	data.s1_grid_time = DBL_MAX;
	data.s2_grid_time = 19.0; // if it didn't hit s1 it will hit grid in 19 seconds
	data.s1_grid_axis = AXIS_NONE;
	data.s2_grid_axis = X_AXIS; // if it didn't hit s1 it will hit grid along x axis
	data.s1_vel_after.x = -1.0; data.s1_vel_after.y = 0.0; data.s1_vel_after.z = 0.0;
	data.s2_vel_after.x = 0.0; data.s2_vel_after.y = 0.0; data.s2_vel_after.z = 0.0;
	data.test_name = "Test one";
	// run test
	test_harness(&data);
}

// Sphere 1 and 2 are moving towards each other.
// Both have the same energy.
// After impact their velocity in the x axis is reversed.
static void test_2() {
	// Set up first sphere
	struct sphere_s s1;
	s1.vel.x = 1.0; s1.vel.y = 0.0; s1.vel.z = 0.0;
	s1.pos.x = 10.0; s1.pos.y = 10.0; s1.pos.z = 10.0;
	s1.mass = 1.0; s1.radius = 1.0;
	// Set up second sphere
	struct sphere_s s2;
	s2.vel.x = -1.0; s2.vel.y = 0.0; s2.vel.z = 0.0;
	s2.pos.x = 20.0; s2.pos.y = 10.0; s2.pos.z = 10.0;
	s2.mass = 1.0; s2.radius = 1.0;
	// Set up grid boundaries
	sim_data.grid_size.x = 50.0;
	sim_data.grid_size.y = 50.0;
	sim_data.grid_size.z = 50.0;
	// Set up expected results
	struct test_data_s data;
	data.s1 = &s1;
	data.s2 = &s2;
	data.col_time = 4.0;
	data.s1_grid_time = 39.0;
	data.s2_grid_time = 19.0;
	data.s1_grid_axis = X_AXIS;
	data.s2_grid_axis = X_AXIS;
	data.s1_vel_after.x = -1.0; data.s1_vel_after.y = 0.0; data.s1_vel_after.z = 0.0;
	data.s2_vel_after.x = 1.0; data.s2_vel_after.y = 0.0; data.s2_vel_after.z = 0.0;
	data.test_name = "Test two";
	// run test
	test_harness(&data);
}

// Same as test_2 along y axis
static void test_3() {
	// Set up first sphere
	struct sphere_s s1;
	s1.vel.x = 0.0; s1.vel.y = 1.0; s1.vel.z = 0.0;
	s1.pos.x = 10.0; s1.pos.y = 10.0; s1.pos.z = 10.0;
	s1.mass = 1.0; s1.radius = 1.0;
	// Set up second sphere
	struct sphere_s s2;
	s2.vel.x = 0.0; s2.vel.y = -1.0; s2.vel.z = 0.0;
	s2.pos.x = 10.0; s2.pos.y = 20.0; s2.pos.z = 10.0;
	s2.mass = 1.0; s2.radius = 1.0;
	// Set up grid boundaries
	sim_data.grid_size.x = 50.0;
	sim_data.grid_size.y = 50.0;
	sim_data.grid_size.z = 50.0;
	// Set up expected results
	struct test_data_s data;
	data.s1 = &s1;
	data.s2 = &s2;
	data.col_time = 4.0;
	data.s1_grid_time = 39.0;
	data.s2_grid_time = 19.0;
	data.s1_grid_axis = Y_AXIS;
	data.s2_grid_axis = Y_AXIS;
	data.s1_vel_after.x = 0.0; data.s1_vel_after.y = -1.0; data.s1_vel_after.z = 0.0;
	data.s2_vel_after.x = 0.0; data.s2_vel_after.y = 1.0; data.s2_vel_after.z = 0.0;
	data.test_name = "Test three";
	// run test
	test_harness(&data);
}

// Same as test_2 along z axis
static void test_4() {
	// Set up first sphere
	struct sphere_s s1;
	s1.vel.x = 0.0; s1.vel.y = 0.0; s1.vel.z = 1.0;
	s1.pos.x = 10.0; s1.pos.y = 10.0; s1.pos.z = 10.0;
	s1.mass = 1.0; s1.radius = 1.0;
	// Set up second sphere
	struct sphere_s s2;
	s2.vel.x = 0.0; s2.vel.y = 0.0; s2.vel.z = -1.0;
	s2.pos.x = 10.0; s2.pos.y = 10.0; s2.pos.z = 20.0;
	s2.mass = 1.0; s2.radius = 1.0;
	// Set up grid boundaries
	sim_data.grid_size.x = 50.0;
	sim_data.grid_size.y = 50.0;
	sim_data.grid_size.z = 50.0;
	// Set up expected results
	struct test_data_s data;
	data.s1 = &s1;
	data.s2 = &s2;
	data.col_time = 4.0;
	data.s1_grid_time = 39.0;
	data.s2_grid_time = 19.0;
	data.s1_grid_axis = Z_AXIS;
	data.s2_grid_axis = Z_AXIS;
	data.s1_vel_after.x = 0.0; data.s1_vel_after.y = 0.0; data.s1_vel_after.z = -1.0;
	data.s2_vel_after.x = 0.0; data.s2_vel_after.y = 0.0; data.s2_vel_after.z = 1.0;
	data.test_name = "Test four";
	// run test
	test_harness(&data);
}


void run_tests() {
	test_1();
	test_2();
	test_3();
	test_4();
}

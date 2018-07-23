#include <stdio.h>
#include <stdlib.h>

static int64_t count;
static FILE *fp;
static const double one = 1.0;

static void create_sphere(double x, double y, double z, double x_vel, double y_vel, double z_vel) {
	fwrite(&count, sizeof(int64_t), 1, fp);
	fwrite(&x, sizeof(double), 1, fp);
	fwrite(&y, sizeof(double), 1, fp);
	fwrite(&z, sizeof(double), 1, fp);
	fwrite(&x_vel, sizeof(double), 1, fp);
	fwrite(&y_vel, sizeof(double), 1, fp);
	fwrite(&z_vel, sizeof(double), 1, fp);
	fwrite(&one, sizeof(double), 1, fp); // mass
	fwrite(&one, sizeof(double), 1, fp); // radius
	count++;
}

// Creates a number of spheres in a line
static void create_spheres(int num, double x_start, double y_start, double z_start, double x_inc, double y_inc, double z_inc) {
	double x = x_start;
	double y = y_start;
	double z = z_start;
	int i;
	for (i = 0; i < num; i++) {
		double xv = rand() / (RAND_MAX + 1.0);
		double yv = rand() / (RAND_MAX + 1.0);
		double zv = rand() / (RAND_MAX + 1.0);
		create_sphere(x, y, z, xv, yv, zv);
		x += x_inc;
		y += y_inc;
		z += z_inc;
	}
}

// Generates 4000 spheres placed in lines in a 1000x1000x1000 grid.
// Each sphere has a random velocity.
static void generate_4000(){
	count = 0;
	srand(123);
	fp = fopen("4000.spheres", "wb");
	double grid_size = 1000.0;
	fwrite(&grid_size, sizeof(double), 1, fp); // x
	fwrite(&grid_size, sizeof(double), 1, fp); // y
	fwrite(&grid_size, sizeof(double), 1, fp); // z
	int64_t num_spheres = 4000;
	fwrite(&num_spheres, sizeof(int64_t), 1, fp);
	create_spheres(250, 10.0, 10.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 40.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 100.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 140.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 360.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 390.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 460.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 490.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 510.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 540.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 610.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 640.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 860.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 890.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 960.0, 10.0, 3.5, 0.0, 0.0);
	create_spheres(250, 10.0, 990.0, 10.0, 3.5, 0.0, 0.0);
	fclose(fp);
}

static void generate_one_axis_crossing_test(){
	count = 0;
	fp = fopen("one_axis.spheres", "wb");
	double grid_size = 100.0;
	fwrite(&grid_size, sizeof(double), 1, fp); // x
	fwrite(&grid_size, sizeof(double), 1, fp); // y
	grid_size = 50.0;
	fwrite(&grid_size, sizeof(double), 1, fp); // z
	int64_t num_spheres = 12;
	fwrite(&num_spheres, sizeof(int64_t), 1, fp);
	// stationary in [0][0][0], moving in [1][0][0]
	create_sphere(49.5, 10.0, 10.0, 0.0, 0.0, 0.0);
	create_sphere(60.5, 10.0, 10.0, -5.0, 0.0, 0.0);
	// moving in [0][0][0], stationary in [1][0][0]
	create_sphere(40.0, 20.0, 10.0, 3.0, 0.0, 0.0);
	create_sphere(50.5, 20.0, 10.0, 0.0, 0.0, 0.0);
	// stationary in [0][0][0], moving in [0][1][0]
	create_sphere(10.0, 49.5, 10.0, 0.0, 0.0, 0.0);
	create_sphere(10.0, 60.5, 10.0, 0.0, -4.0, 0.0);
	// moving in [0][0][0], stationary in [0][1][0]
	create_sphere(20.0, 40.0, 10.0, 0.0, 2.0, 0.0);
	create_sphere(20.0, 50.5, 10.0, 0.0, 0.0, 0.0);
	// moving in [0][0][0], stationary in [0][0][1]
	create_sphere(10.0, 10.0, 23.0, 0.0, 0.0, 2.0);
	create_sphere(10.0, 10.0, 25.5, 0.0, 0.0, 0.0);
	// stationary in [0][0][0], moving in [0][0][1]
	create_sphere(5.0, 10.0, 24.9, 0.0, 0.0, 0.0);
	create_sphere(5.0, 10.0, 27, 0.0, 0.0, -2.0);
	fclose(fp);
}

static void generate_two_axis_crossing_test(){
	count = 0;
	fp = fopen("two_axis.spheres", "wb");
	double grid_size = 100.0;
	fwrite(&grid_size, sizeof(double), 1, fp); // x
	fwrite(&grid_size, sizeof(double), 1, fp); // y
	grid_size = 50.0;
	fwrite(&grid_size, sizeof(double), 1, fp); // z
	int64_t num_spheres = 18;
	fwrite(&num_spheres, sizeof(int64_t), 1, fp);
	create_sphere(40.0, 40.0, 10.0, 5.5, 5.5, 0.0);
	create_sphere(60.0, 60.0, 10.0, -5.1, -5.1, 0.0);
	create_sphere(40.0, 60.0, 10.0, 3.5, -3.5, 0.0);
	create_sphere(60.0, 40.0, 10.0, -3.1, 3.1, 0.0);
	create_sphere(45.0, 30.0, 20.0, 3.0, 0.0, 3.0);
	create_sphere(55.0, 30.0, 30.0, -3.1, 0.0, -3.1);
	create_sphere(45.0, 20.0, 20.0, 3.1, 0.0, 3.1);
	create_sphere(55.0, 20.0, 30.0, -3.0, 0.0, -3.0);
	create_sphere(55.0, 70.0, 20.0, -3.0, 0.0, 3.0);
	create_sphere(45.0, 70.0, 30.0, 3.1, 0.0, -3.1);
	create_sphere(5.0, 45.0, 20.0, 0.0, 2.0, 2.0);
	create_sphere(5.0, 55.0, 30.0, 0.0, -2.1, -2.1);
	create_sphere(10.0, 45.0, 20.0, 0.0, 2.2, 2.2);
	create_sphere(10.0, 55.0, 30.0, 0.0, -2.1, -2.1);
}

static void generate_three_axis_crossing_test(){
	count = 0;
	fp = fopen("three_axis.spheres", "wb");
	double grid_size = 90.0;
	fwrite(&grid_size, sizeof(double), 1, fp); // x
	fwrite(&grid_size, sizeof(double), 1, fp); // y
	fwrite(&grid_size, sizeof(double), 1, fp); // z
	int64_t num_spheres = 16;
	fwrite(&num_spheres, sizeof(int64_t), 1, fp);
	create_sphere(25.0, 65.0, 65.0, 2.0, -2.0, -2.0);
	create_sphere(35.0, 55.0, 55.0, -2.1, 2.1, 2.1);
	create_sphere(55.0, 55.0, 55.0, 2.1, 2.1, 2.1);
	create_sphere(65.0, 65.0, 65.0, -2.0, -2.0, -2.0);
	create_sphere(65.0, 65.0, 25.0, -2.0, -2.0, 2.0);
	create_sphere(55.0, 55.0, 35.0, 2.1, 2.1, -2.1);
	create_sphere(25.0, 65.0, 25.0, 2.0, -2.0, 2.0);
	create_sphere(35.0, 55.0, 35.0, -2.1, 2.1, -2.1);
	create_sphere(25.0, 25.0, 65.0, 2.0, 2.0, -2.0);
	create_sphere(35.0, 35.0, 55.0, -2.1, -2.1, 2.1);
	create_sphere(65.0, 25.0, 65.0, -2.0, 2.0, -2.0);
	create_sphere(55.0, 35.0, 55.0, 2.1, -2.1, 2.1);
	create_sphere(65.0, 25.0, 25.0, -2.0, 2.0, 2.0);
	create_sphere(55.0, 35.0, 35.0, 2.1, -2.1, -2.1);
	create_sphere(25.0, 25.0, 25.0, 2.0, 2.0, 2.0);
	create_sphere(35.0, 35.0, 35.0, -2.1, -2.1, -2.1);
}

// This will generate some inital configs used by the collision simulator.
int main(void){
	generate_4000();
	generate_one_axis_crossing_test();
	generate_two_axis_crossing_test();	
	generate_three_axis_crossing_test();
}


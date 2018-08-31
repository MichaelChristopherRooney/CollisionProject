#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

static const double mod = 100.0;
// Creates a number of spheres in a line
static void create_spheres(int num, double x_start, double y_start, double z_start, double x_inc, double y_inc, double z_inc) {
	double x = x_start;
	double y = y_start;
	double z = z_start;
	int i;
	for (i = 0; i < num; i++) {
		double xv = ((drand48() - 0.5) * 2.0) * mod;
		double yv = ((drand48() - 0.5) * 2.0) * mod;
		double zv = ((drand48() - 0.5) * 2.0) * mod;
		create_sphere(x, y, z, xv, yv, zv);
		x += x_inc;
		y += y_inc;
		z += z_inc;
	}
}

// Simple test with one sphere going back and forth
static void generate_transfer(){
	count = 0;
	srand48(123);
	fp = fopen("transfer.spheres", "wb");
	double grid_size = 100.0;
	fwrite(&grid_size, sizeof(double), 1, fp); // x
	fwrite(&grid_size, sizeof(double), 1, fp); // y
	fwrite(&grid_size, sizeof(double), 1, fp); // z
	int64_t num_spheres = 1;
	fwrite(&num_spheres, sizeof(int64_t), 1, fp);
	create_sphere(60.0, 40.0, 10.0, -500.0, 0.0, 0.0);
	fclose(fp);
}

// Generates 4000 spheres placed in lines in a 1000x1000x1000 grid.
// Each sphere has a random velocity.
static void generate_4000(){
	count = 0;
	srand48(123);
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

// Generates 10000 spheres placed in lines in a 1000x1000x1000 grid.
// Each sphere has a random velocity.
static void generate_10000(){
	count = 0;
	srand48(123);
	fp = fopen("10000.spheres", "wb");
	double grid_size = 1000.0;
	fwrite(&grid_size, sizeof(double), 1, fp); // x
	fwrite(&grid_size, sizeof(double), 1, fp); // y
	fwrite(&grid_size, sizeof(double), 1, fp); // z
	int64_t num_spheres = 10000;
	fwrite(&num_spheres, sizeof(int64_t), 1, fp);
	int i, j;
	double y, z;
	z = 10.0;
	for(i = 0; i < 10; i++){
		y = 10.0;
		for(j = 0; j < 20; j++){
			create_spheres(250, 10.0, y, z, 3.95, 0.0, 0.0);
			create_spheres(250, 10.0, 500.0 + y, z, 3.95, 0.0, 0.0);
			y += 10.0;
		}
		z += 10.0;
	}
	fclose(fp);
}

// Generates 50176 spheres placed in lines.
// Each sphere has a random velocity.
static void generate_50000(){
	count = 0;
	srand48(123);
	fp = fopen("50000.spheres", "wb");
	double grid_size = 500.0;
	fwrite(&grid_size, sizeof(double), 1, fp); // x
	fwrite(&grid_size, sizeof(double), 1, fp); // y
	fwrite(&grid_size, sizeof(double), 1, fp); // z
	int64_t num_spheres = 50176;
	fwrite(&num_spheres, sizeof(int64_t), 1, fp);
	int i, j;
	double x, y, z;
	const double offset = 1.5;
	const double inc = (grid_size - (offset * 2.0)) / sqrt(num_spheres);
	printf("inc is %.17g\n", inc);
	const double num_in_line = 224;
	z = 10.0;
	for(i = 0; i < num_in_line; i++){
		y = 2.0 + (inc * i);
		create_spheres(num_in_line, 2.0, y, z, inc, 0.0, 0.0);
	}
	fclose(fp);
}

// Generates 50176 spheres placed in lines.
// Each sphere has a random velocity.
static void generate_50000_sparse(){
	count = 0;
	srand48(123);
	fp = fopen("50000_sparse.spheres", "wb");
	double grid_size = 50000.0;
	fwrite(&grid_size, sizeof(double), 1, fp); // x
	fwrite(&grid_size, sizeof(double), 1, fp); // y
	fwrite(&grid_size, sizeof(double), 1, fp); // z
	int64_t num_spheres = 50176;
	fwrite(&num_spheres, sizeof(int64_t), 1, fp);
	int i, j;
	double x, y, z;
	const double offset = 1.5;
	const double inc = (grid_size - (offset * 2.0)) / sqrt(num_spheres);
	printf("inc is %.17g\n", inc);
	const double num_in_line = 224;
	z = 10.0;
	for(i = 0; i < num_in_line; i++){
		y = 2.0 + (inc * i);
		create_spheres(num_in_line, 2.0, y, z, inc, 0.0, 0.0);
	}
	fclose(fp);
}
// Generates 500'000 spheres placed in lines.
// Each sphere has a random velocity.
static void generate_500k(){
	count = 0;
	srand48(123);
	fp = fopen("500k.spheres", "wb");
	double grid_size = 1750.0;
	fwrite(&grid_size, sizeof(double), 1, fp); // x
	fwrite(&grid_size, sizeof(double), 1, fp); // y
	fwrite(&grid_size, sizeof(double), 1, fp); // z
	int64_t num_spheres = 501264;
	fwrite(&num_spheres, sizeof(int64_t), 1, fp);
	int i, j;
	double x, y, z;
	const double offset = 1.5;
	const double inc = (grid_size - (offset * 2.0)) / sqrt(num_spheres);
	printf("inc is %.17g\n", inc);
	const double num_in_line = 708;
	z = 10.0;
	for(i = 0; i < num_in_line; i++){
		y = 2.0 + (inc * i);
		create_spheres(num_in_line, 2.0, y, z, inc, 0.0, 0.0);
	}
	fclose(fp);
}

// Generates 1'000'000 spheres placed in lines.
// Each sphere has a random velocity.
static void generate_1mil(){
	count = 0;
	srand48(123);
	fp = fopen("1mil.spheres", "wb");
	double grid_size = 2200.0;
	fwrite(&grid_size, sizeof(double), 1, fp); // x
	fwrite(&grid_size, sizeof(double), 1, fp); // y
	fwrite(&grid_size, sizeof(double), 1, fp); // z
	int64_t num_spheres = 1000000;
	fwrite(&num_spheres, sizeof(int64_t), 1, fp);
	int i, j;
	double x, y, z;
	const double offset = 1.75;
	const double inc = (grid_size - (offset * 2.0)) / sqrt(num_spheres);
	printf("inc is %.17g\n", inc);
	const double num_in_line = 1000;
	z = 10.0;
	for(i = 0; i < num_in_line; i++){
		y = 2.0 + (inc * i);
		create_spheres(num_in_line, 2.0, y, z, inc, 0.0, 0.0);
	}
	fclose(fp);
}

// Generates 2'502'724 spheres placed in lines.
// Each sphere has a random velocity.
static void generate_2mil(){
	count = 0;
	srand48(123);
	fp = fopen("2mil.spheres", "wb");
	double grid_size = 3750.0;
	fwrite(&grid_size, sizeof(double), 1, fp); // x
	fwrite(&grid_size, sizeof(double), 1, fp); // y
	fwrite(&grid_size, sizeof(double), 1, fp); // z
	int64_t num_spheres = 2502724;
	fwrite(&num_spheres, sizeof(int64_t), 1, fp);
	int i, j;
	double x, y, z;
	const double offset = 1.25;
	const double inc = (grid_size - (offset * 2.0)) / sqrt(num_spheres);
	printf("inc is %.17g\n", inc);
	const double num_in_line = 1582;
	z = 10.0;
	for(i = 0; i < num_in_line; i++){
		y = 2.0 + (inc * i);
		create_spheres(num_in_line, 2.0, y, z, inc, 0.0, 0.0);
	}
	fclose(fp);
}

// Generates 5'004'169 spheres placed in lines.
// Each sphere has a random velocity.
static void generate_5mil(){
	count = 0;
	srand48(123);
	fp = fopen("5mil.spheres", "wb");
	double grid_size = 4750.0;
	fwrite(&grid_size, sizeof(double), 1, fp); // x
	fwrite(&grid_size, sizeof(double), 1, fp); // y
	fwrite(&grid_size, sizeof(double), 1, fp); // z
	int64_t num_spheres = 2237 * 2237;
	fwrite(&num_spheres, sizeof(int64_t), 1, fp);
	int i, j;
	double x, y, z;
	const double offset = 1.25;
	const double inc = (grid_size - (offset * 2.0)) / sqrt(num_spheres);
	printf("inc is %.17g\n", inc);
	const double num_in_line = 2237;
	z = 10.0;
	for(i = 0; i < num_in_line; i++){
		y = 2.0 + (inc * i);
		create_spheres(num_in_line, 2.0, y, z, inc, 0.0, 0.0);
	}
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
	int64_t num_spheres = 13;
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
	generate_10000();
	generate_50000();
	generate_50000_sparse();
	generate_500k();
	generate_1mil();
	generate_2mil();
	generate_5mil();
	generate_one_axis_crossing_test();
	generate_two_axis_crossing_test();	
	generate_three_axis_crossing_test();
	generate_transfer();
}


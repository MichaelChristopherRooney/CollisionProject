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
		count++;
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

// This will generate some inital configs used by the collision simulator.
int main(void){
	generate_4000();
}


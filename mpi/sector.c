#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "event.h"
#include "mpi_vars.h"
#include "simulation.h"
#include "sector.h"

// Used when iterating over axes and nned to access sector adjacent on the current axis.
const int SECTOR_MODIFIERS[2][3][3] = {
	{
		{ 1, 0, 0 }, // x
		{ 0, 1, 0 }, // y
		{ 0, 0, 1 }, // z
	},
	{
		{ -1, 0, 0 }, // x
		{ 0, -1, 0 }, // y
		{ 0, 0, -1 }, // z
	}
};

static void set_largest_radius_after_insertion(struct sector_s *sector, const struct sphere_s *sphere) {
	if (sphere->radius == sector->largest_radius) {
		sector->largest_radius_shared = true; // Quicker to just set it rather than check if already set
		sector->num_largest_radius_shared++;
	} else if (sphere->radius > sector->largest_radius) {
		sector->largest_radius = sphere->radius;
	}
}

void add_sphere_to_sector(struct sector_s *sector, const struct sphere_s *sphere) {
	if (sector->num_spheres >= sector->max_spheres) {
		printf("!!! need to resize\n");
		sector->max_spheres = sector->max_spheres * 2;
		sector->spheres = realloc(sector->spheres, sector->max_spheres * sizeof(struct sphere_s));
	}
	sector->spheres[sector->num_spheres] = *sphere;
	sector->spheres[sector->num_spheres].sector_id = sector->num_spheres;
	sector->num_spheres++;
	set_largest_radius_after_insertion(sector, sphere);
}

static void set_largest_radius_after_removal(struct sector_s *sector, const struct sphere_s *sphere) {
	if (sphere->radius == sector->largest_radius && sector->largest_radius_shared == false && sector->num_spheres > 0) {
		// TODO: search for new largest radius
		printf("TODO: find new largest radius\n");
		getchar();
		exit(1);
	} else if (sphere->radius == sector->largest_radius && sector->largest_radius_shared == true) {
		sector->num_largest_radius_shared--;
		if (sector->num_largest_radius_shared == 0) {
			sector->largest_radius_shared = false;
		}
	} else if (sector->num_spheres == 0) {
		sector->largest_radius = 0.0;
	}
}

static void shift(struct sector_s *sector, const struct sphere_s *sphere) {
	int64_t i;
	for (i = sphere->sector_id; i < sector->num_spheres - 1; i++) {
		sector->spheres[i] = sector->spheres[i + 1];
		sector->spheres[i].sector_id--;
	}
}

// Each sphere has a sector id which gives its location in the sector's sphere
// array.
// If the sphere to be removed is the only or last it can just be removed normally.
// Otherwise each sphere after it in the array must be shifted down by one and
// have its own sector id decremented. 
// I previously tried doing this without a sector id. A linear search followed 
// by a memmove was done, but this was not any faster with 4'000 spheres.
// It may be faster when dealing with a much larger number of spheres however.
void remove_sphere_from_sector(struct sector_s *sector, const struct sphere_s *sphere) {
	if (sector->num_spheres == 1 || sector->num_spheres - 1 == sphere->sector_id) {
		sector->num_spheres--;
		set_largest_radius_after_removal(sector, sphere);
		return;
	}
	shift(sector, sphere);
	sector->num_spheres--;
	set_largest_radius_after_removal(sector, sphere);
}

// Given a sector returns the sector adjacent in specified direction on the
// specified axis.
// Ex: If x axis and positive direction returns sector to the right.
struct sector_s *get_adjacent_sector_non_diagonal(const struct sector_s *sector, const enum axis a, const enum direction dir) {

	int x = sector->pos.x + SECTOR_MODIFIERS[dir][a][X_AXIS];
	int y = sector->pos.y + SECTOR_MODIFIERS[dir][a][Y_AXIS];
	int z = sector->pos.z + SECTOR_MODIFIERS[dir][a][Z_AXIS];
	struct sector_s *adj = &sim_data.sectors[x][y][z];
	return adj;
}

// Helper for add_sphere_to_correct_sector function.
// Also used when MPI code is loading sphere data from file.
bool does_sphere_belong_to_sector(const struct sphere_s *sphere, const struct sector_s *sector) {
	enum axis a;
	for (a = X_AXIS; a <= Z_AXIS; a++) {
		if (sphere->pos.vals[a] < sector->start.vals[a] || sphere->pos.vals[a] >= sector->end.vals[a]) {
			return false;
		}
	}
	return true;
}

struct sector_s *find_sector_that_sphere_belongs_to(struct sphere_s *sphere){
	int x, y, z;
	for (x = 0; x < sim_data.sector_dims[X_AXIS]; x++) {
		for (y = 0; y < sim_data.sector_dims[Y_AXIS]; y++) {
			for (z = 0; z < sim_data.sector_dims[Z_AXIS]; z++) {
				bool res = does_sphere_belong_to_sector(sphere, &sim_data.sectors[x][y][z]);
				if (res) {
					return &sim_data.sectors[x][y][z];
				}
			}
		}
	}
	// Shouldn't reach here
	printf("Error: sphere does not belong to any sector\n");
	exit(1);
}

static void init_local_files_for_file_backed_memory(){
	SECTOR->spheres_filename = calloc(SECTOR_MAX_FILENAME_LENGTH, sizeof(char));
	SECTOR->size_filename = calloc(SECTOR_MAX_FILENAME_LENGTH, sizeof(char));
	sprintf(SECTOR->size_filename, "%d-size.bin", SECTOR->id);
	sprintf(SECTOR->spheres_filename, "%d-sphere.bin", SECTOR->id);
	SECTOR->size_fd = open(SECTOR->size_filename, O_CREAT | O_RDWR, S_IRWXU);
	SECTOR->spheres_fd = open(SECTOR->spheres_filename, O_CREAT | O_RDWR, S_IRWXU);
	int res = ftruncate(SECTOR->spheres_fd, SECTOR_DEFAULT_MAX_SPHERES * sizeof(struct sphere_s));
	if(res == -1){
		printf("Error calling ftruncate: %s\n", strerror(errno));
	}
}

// Check if the passed sector is a neighbour to the local sector
// It's a neighbour if it is within 0 or 1 unit in all directions.
static bool check_is_neighbour(struct sector_s *s){
	int x_dist = abs(SECTOR->pos.x - s->pos.x);
	int y_dist = abs(SECTOR->pos.y - s->pos.y);
	int z_dist = abs(SECTOR->pos.z - s->pos.z);
	return x_dist <= 1 && y_dist <= 1 && z_dist <= 1;
}

static void alloc_sector_array(){
	sim_data.sectors = calloc(sim_data.sector_dims[X_AXIS], sizeof(struct sector_s **));
	sim_data.sectors_flat = calloc(sim_data.sector_dims[X_AXIS] * sim_data.sector_dims[Y_AXIS] * sim_data.sector_dims[Z_AXIS], sizeof(struct sector_s));
	int i, j;
	for (i = 0; i < sim_data.sector_dims[X_AXIS]; i++) {
		sim_data.sectors[i] = calloc(sim_data.sector_dims[Y_AXIS], sizeof(struct sector_s *));
		for (j = 0; j < sim_data.sector_dims[Y_AXIS]; j++) {
			int idx = (i * sim_data.sector_dims[Y_AXIS] * sim_data.sector_dims[Z_AXIS]) + (j * sim_data.sector_dims[Z_AXIS]);
			sim_data.sectors[i][j] = &sim_data.sectors_flat[idx];
		}
	}
}

static void set_local_sector(){
	SECTOR = &sim_data.sectors[COORDS[X_AXIS]][COORDS[Y_AXIS]][COORDS[Z_AXIS]];
	SECTOR->id = GRID_RANK;
	SECTOR->pos.x = COORDS[X_AXIS];
	SECTOR->pos.y = COORDS[Y_AXIS];
	SECTOR->pos.z = COORDS[Z_AXIS];
	init_local_files_for_file_backed_memory();
}

static void set_sectors(){
	set_local_sector();
	double x_inc = sim_data.grid_size.x / sim_data.sector_dims[X_AXIS];
	double y_inc = sim_data.grid_size.y / sim_data.sector_dims[Y_AXIS];
	double z_inc = sim_data.grid_size.z / sim_data.sector_dims[Z_AXIS];
	int id = 0;
	int i, j, k;
	char recv_hn[MAX_HOSTNAME_LENGTH];
	char send_hn[MAX_HOSTNAME_LENGTH];
	char spheres_fn_recv[SECTOR_MAX_FILENAME_LENGTH];
	char size_fn_recv[SECTOR_MAX_FILENAME_LENGTH];
	gethostname(send_hn, MAX_HOSTNAME_LENGTH);
	for (i = 0; i < sim_data.sector_dims[X_AXIS]; i++) {
		for (j = 0; j < sim_data.sector_dims[Y_AXIS]; j++) {
			for (k = 0; k < sim_data.sector_dims[Z_AXIS]; k++) {
				struct sector_s *s = &sim_data.sectors[i][j][k];
				s->num_spheres = 0;
				s->max_spheres = SECTOR_DEFAULT_MAX_SPHERES;
				s->start.x = x_inc * i;
				s->end.x = s->start.x + x_inc;
				s->start.y = y_inc * j;
				s->end.y = s->start.y + y_inc;
				s->start.z = z_inc * k;
				s->end.z = s->start.z + z_inc;
				if(s == SECTOR){
					MPI_Bcast(send_hn, MAX_HOSTNAME_LENGTH, MPI_CHAR, id, GRID_COMM);
					MPI_Bcast(SECTOR->size_filename, SECTOR_MAX_FILENAME_LENGTH, MPI_CHAR, id, GRID_COMM);
					MPI_Bcast(SECTOR->spheres_filename, SECTOR_MAX_FILENAME_LENGTH, MPI_CHAR, id, GRID_COMM);
					SECTOR->spheres = mmap(NULL, SECTOR->max_spheres * sizeof(struct sphere_s), PROT_READ | PROT_WRITE, MAP_SHARED, SECTOR->spheres_fd, 0);
					if(SECTOR->spheres == NULL || SECTOR->spheres == (void *) -1){
						printf("%s\n", strerror(errno));
					}
				} else {
					s->id = id;
					s->pos.x = i;
					s->pos.y = j;
					s->pos.z = k;
					MPI_Bcast(recv_hn, MAX_HOSTNAME_LENGTH, MPI_CHAR, id, GRID_COMM);
					MPI_Bcast(size_fn_recv, SECTOR_MAX_FILENAME_LENGTH, MPI_CHAR, id, GRID_COMM);
					MPI_Bcast(spheres_fn_recv, SECTOR_MAX_FILENAME_LENGTH, MPI_CHAR, id, GRID_COMM);
					if(check_is_neighbour(s)){
						s->is_neighbour = true;
						NEIGHBOUR_IDS[NUM_NEIGHBOURS] = s->id;
						NUM_NEIGHBOURS++;
						if(strcmp(recv_hn, send_hn) == 0){
							s->is_local_neighbour = true;
							s->size_fd = open(size_fn_recv, O_CREAT | O_RDWR, S_IRWXU);
							s->spheres_fd = open(spheres_fn_recv, O_CREAT | O_RDWR, S_IRWXU);
							s->spheres = mmap(NULL, s->max_spheres * sizeof(struct sphere_s), PROT_READ | PROT_WRITE, MAP_SHARED, s->spheres_fd, 0);
							if(s->spheres == NULL || s->spheres == (void *) -1){
								printf("%s\n", strerror(errno));
							}
						} else {
							s->spheres = calloc(s->max_spheres, sizeof(struct sphere_s));
						}
					}
				}
				id++;	
			}
		}
	}
}

void init_sectors(){
	sim_data.num_sectors = sim_data.sector_dims[X_AXIS] * sim_data.sector_dims[Y_AXIS] * sim_data.sector_dims[Z_AXIS];
	sim_data.xy_check_needed = sim_data.sector_dims[X_AXIS] > 1 && sim_data.sector_dims[Y_AXIS] > 1;
	sim_data.xz_check_needed = sim_data.sector_dims[X_AXIS] > 1 && sim_data.sector_dims[Z_AXIS] > 1;
	sim_data.yz_check_needed = sim_data.sector_dims[Y_AXIS] > 1 && sim_data.sector_dims[Z_AXIS] > 1;
	alloc_sector_array();
	set_sectors();
	MPI_Barrier(GRID_COMM);
}


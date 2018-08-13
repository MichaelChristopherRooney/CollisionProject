#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "event.h"
#include "grid.h"
#include "io.h"
#include "mpi_vars.h"
#include "params.h"
#include "simulation.h"
#include "vector_3.h"


/*
Need to rewrite this for MPI version
static void compare_results() {
	if(compare_file == NULL){
		return;
	}
	FILE *fp = fopen(compare_file, "rb");
	int64_t comp_num_spheres;
	// result is just to shut up gcc's warnings
	int result = fread(&comp_num_spheres, sizeof(int64_t), 1, fp);
	if(comp_num_spheres != NUM_SPHERES){
		printf("Error: cannot compare files as number of spheres differs\n");
		return;
	}
	double max_pos_err = 0.0;
	double max_vel_err = 0.0;
	int i;
	for (i = 0; i < NUM_SPHERES; i++) {
		struct sphere_s s = spheres[i];
		union vector_3d vel_comp;
		union vector_3d pos_comp;
		result = fread(&vel_comp, sizeof(union vector_3d), 1, fp);
		result = fread(&pos_comp, sizeof(union vector_3d), 1, fp);
		enum axis a;
		for (a = X_AXIS; a <= Z_AXIS; a++) {
			if (fabs(s.pos.vals[a] - pos_comp.vals[a]) > max_pos_err) {
				max_pos_err = fabs(s.pos.vals[a] - pos_comp.vals[a]);
			}
			if (fabs(s.vel.vals[a] - vel_comp.vals[a]) > max_vel_err) {
				max_vel_err = fabs(s.vel.vals[a] - vel_comp.vals[a]);
			}
		}
	}
	printf("vel abs err: %.17g\n", max_vel_err);
	printf("pos abs err: %.17g\n", max_pos_err);
}
*/

// For debugging
// Helps catch any issues with transfering spheres between sectors.
void sanity_check() {
	static const double eps = 10E-12;
	int64_t i, j;
	for(i = 0; i < sim_data.num_sectors; i++){
		struct sector_s *s = &sim_data.sectors_flat[i];
		bool cont = ALL_HELP || (s->is_neighbour && !s->is_local_neighbour) || SECTOR->id == s->id;
		if(!cont){
			continue;
		}
		for(j = 0; j < s->num_spheres; j++){
			struct sphere_s *sphere = &s->spheres[j];
			int error = 0;
			enum axis a;
			for (a = X_AXIS; a <= Z_AXIS; a++) {
				double e1 = sphere->pos.vals[a] - s->end.vals[a];
				double e2 = s->start.vals[a] - sphere->pos.vals[a];
				if (e1 > eps || e2 > eps) {
					error = 1;
				}
				if (sphere->pos.vals[a] < 0.0 || sphere->pos.vals[a] > sim_data.grid_size.vals[a]) {
					error = 1;
				}
			}
			if (error) {
				printf("Sector at %d, %d, %d with pos %f, %f, %f, incorrectly has sphere %ld with pos %f, %f, %f\n", s->pos.x, s->pos.y, s->pos.z, s->start.x, s->start.y, s->start.z, sphere->id, sphere->pos.x, sphere->pos.y, sphere->pos.z);
				exit(1);
			}
		}

	}
}

// Old output or final state files may be present if names are reused.
// This has been causing issues so delete them here.
// Note: files for file backed memory for each sector are deleted later.
static void delete_old_files(){
	if(GRID_RANK == 0){
		if(final_state_file != NULL){
			unlink(final_state_file);
		}
		unlink(output_file);
	}
	MPI_Barrier(GRID_COMM);
}

static void init_stats(){
	stats.num_two_sphere_collisions = 0;
	stats.num_grid_collisions = 0;
	stats.num_sector_transfers = 0;
	stats.num_partial_crossings = 0;
}

static void parse_args_and_init_mpi(int argc, char *argv[]){
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &WORLD_RANK);
	MPI_Comm_size(MPI_COMM_WORLD, &NUM_NODES);
	parse_args(argc, argv);
	MPI_Cart_create(MPI_COMM_WORLD, NUM_DIMS, sim_data.sector_dims, PERIODS, REORDER, &GRID_COMM);
	MPI_Comm_rank(GRID_COMM, &GRID_RANK);
	MPI_Cart_coords(GRID_COMM, GRID_RANK, NUM_DIMS, COORDS);
}

void simulation_init(int argc, char *argv[]) {
	parse_args_and_init_mpi(argc, argv);
	init_stats();
	sim_data.iteration_number = 0;
	sim_data.elapsed_time = 0.0;
	delete_old_files();
	init_output_file();
	FILE *initial_state_fp = fopen(initial_state_file, "rb");
	init_grid(initial_state_fp);
	init_sectors();
	load_spheres(initial_state_fp);
	fclose(initial_state_fp);
	MPI_Barrier(MPI_COMM_WORLD); // barrier to ensure ftruncate has been called before next step
	check_for_resizing_after_sphere_loading();
	init_events();

}

static void do_grid_iteration(){
	sanity_check();
	// Now find event + time of event
	// Final event may take place after time limit, so cut it short if so
	find_event_times();
	reduce_events();
	if (sim_data.time_limit - sim_data.elapsed_time < next_event->time) {
		next_event->time = sim_data.time_limit - sim_data.elapsed_time;
		if(GRID_RANK == 0){
			MPI_Status s;
			MPI_File_write(MPI_OUTPUT_FILE, &sim_data.time_limit, 1, MPI_DOUBLE, &s);
		}
		update_my_spheres(); // last iteration, so don't care about other sector's spheress
	} else {
		update_spheres();
		apply_event();
	}
	sanity_check();
	sim_data.elapsed_time += next_event->time;
}

static void print_stats(){
	if(GRID_RANK != 0){
		return;
	}
	printf("Number of sphere on sphere collisions: %d\n", stats.num_two_sphere_collisions);
	printf("Number of collisions with grid boundary: %d\n", stats.num_grid_collisions);
	printf("Number of transfers between sectors: %d\n", stats.num_sector_transfers);
	printf("Number of partial crossings: %d\n", stats.num_partial_crossings);
}

void simulation_run() {
	sim_data.iteration_number = 1; // start at 1 as 0 is iteration num for the initial state
	while (sim_data.elapsed_time < sim_data.time_limit) {
		do_grid_iteration();
		MPI_Barrier(GRID_COMM);
		sim_data.iteration_number++;
	}
	save_final_state_file();
	print_stats();
	//compare_results();
}

void simulation_cleanup() {
	int i;
	for(i = 0; i < sim_data.num_sectors; i++){
		struct sector_s *s = &sim_data.sectors_flat[i];
		if(SECTOR == s){
			munmap(s->spheres, s->max_spheres * sizeof(struct sphere_s));
			unlink(s->spheres_filename);
			close(s->spheres_fd);
		} else if(s->is_local_neighbour){
			munmap(s->spheres, s->max_spheres * sizeof(struct sphere_s));
			//close(s->size_fd);
			close(s->spheres_fd);
		} else if(s->is_neighbour){
			free(s->spheres);
		}
	}
	free(sim_data.sectors[0][0]);
	for (i = 0; i < sim_data.sector_dims[X_AXIS]; i++) {
		free(sim_data.sectors[i]);
	}
	free(sim_data.sectors);
	MPI_File_close(&MPI_OUTPUT_FILE);
	MPI_Comm_free(&GRID_COMM);
}


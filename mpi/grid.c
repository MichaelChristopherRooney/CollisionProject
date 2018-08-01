#include <float.h>
#include <stdlib.h>
#include <stdio.h>

#include "collision.h"
#include "event.h"
#include "grid.h"
#include "io.h"
#include "mpi_vars.h"
#include "params.h"
#include "simulation.h"
#include "vector_3.h"

// Loads the grid from the initial state file
void init_grid() {
	FILE *initial_state_fp = fopen(initial_state_file, "rb");
	// result is just to shut up gcc's warnings
	int result = fread(&sim_data.grid_size.x, sizeof(double), 1, initial_state_fp);
	result = fread(&sim_data.grid_size.y, sizeof(double), 1, initial_state_fp);
	result = fread(&sim_data.grid_size.z, sizeof(double), 1, initial_state_fp);
	write_grid_dimms();
	init_sectors();
	load_spheres(initial_state_fp);
	MPI_Barrier(MPI_COMM_WORLD); // barrier to ensure ftruncate has been called before next step
	check_for_resizing_after_sphere_loading();
	init_events();
	fclose(initial_state_fp);
}

// This updates the positions and velocities of each sphere once the next
// event and the time it occurs are known.
// First update own spheres, then update local copy of neighbour's spheres
// Don't update neighbour's spheres if they are shared via shared memory.
// Also update received copies of sphere(s) involved in next event.
// As they are copies the same sphere is not updated twice.
static void update_spheres() {
	int i, j;
	for (i = 0; i < SECTOR->num_spheres; i++) {
		struct sphere_s *s = &(SECTOR->spheres[i]);
		update_sphere_position(s, next_event->time);
	}
	for(i = 0; i < NUM_NEIGHBOURS; i++){
		struct sector_s *sector = &sim_data.sectors_flat[NEIGHBOUR_IDS[i]];
		if(sector->is_local_neighbour){
			continue;
		}
		for(j = 0; j < sector->num_spheres; j++){
			struct sphere_s *s = &(sector->spheres[j]);
			update_sphere_position(s, next_event->time);
		}
	}
	update_sphere_position(&next_event->sphere_1, next_event->time);
	update_sphere_position(&next_event->sphere_2, next_event->time);
}

// Apply events and write changes to file.
// In each case the source sector is responsible for writing data.
// Each other sector must update their file pointer however.
// TODO: clean this up
static void apply_event(){
	if (next_event->type == COL_SPHERE_WITH_GRID) {
		struct sector_s *source = &sim_data.sectors_flat[next_event->source_sector_id];
		if((source->is_neighbour && !source->is_local_neighbour) || source->id == SECTOR->id){
			struct sphere_s *sphere = &source->spheres[next_event->sphere_1.sector_id];
			sphere->vel.vals[event_details.grid_axis] *= -1.0;
			if(source->id == SECTOR->id){
				write_iteration_data(sphere, NULL);
			}
		}
		if(source->id != SECTOR->id){
			seek_one_sphere();
		}
	} else if (next_event->type == COL_TWO_SPHERES) {
		struct sector_s *source = &sim_data.sectors_flat[next_event->source_sector_id];
		if((source->is_neighbour && !source->is_local_neighbour) || SECTOR->id == next_event->source_sector_id){
			struct sphere_s *s1 = &source->spheres[next_event->sphere_1.sector_id];
			struct sphere_s *s2 = &source->spheres[next_event->sphere_2.sector_id];
			apply_bounce_between_spheres(s1, s2);
			if(source->id == SECTOR->id){
				write_iteration_data(s1, s2);
			}
		}
		if(source->id != SECTOR->id){
			seek_two_spheres();
		}
	} else if (next_event->type == COL_SPHERE_WITH_SECTOR) {
		// Only used if a local neighbour resizes and the local file backed memory needs to be remapped.
		bool resize_needed = false; 
		struct sphere_s *sphere = &next_event->sphere_1;
		struct sector_s *source = &sim_data.sectors_flat[next_event->source_sector_id];
		if(source->is_local_neighbour){
			source->num_spheres--;
			set_largest_radius_after_removal(source, sphere);
		} else if(source->is_neighbour || SECTOR->id == next_event->source_sector_id){
			remove_sphere_from_sector(source, sphere);
		}
		struct sector_s *dest = &sim_data.sectors_flat[next_event->dest_sector_id];
		if(dest->is_local_neighbour){
			dest->num_spheres++;
			set_largest_radius_after_insertion(dest, sphere);
			if(dest->num_spheres >= dest->max_spheres){
				resize_needed = true;
			}
		} else if(dest->is_neighbour || SECTOR->id == next_event->dest_sector_id){
			add_sphere_to_sector(dest, sphere);
			if(dest->id == SECTOR->id){
				write_iteration_data(sphere, NULL);
			}
		}
		if(dest->id != SECTOR->id){
			seek_one_sphere();
		}
		MPI_Barrier(GRID_COMM); 
		// If resize_needed is true then the above barrier ensures the
		// process responsible for the sector has already called ftruncate.
		if(resize_needed){
			resize_sphere_array(dest);
		}
	} else if(next_event->type == COL_TWO_SPHERES_PARTIAL_CROSSING){
		struct sphere_s *s1 = NULL;
		struct sphere_s *s2 = NULL;
		struct sector_s *source = &sim_data.sectors_flat[next_event->source_sector_id];
		struct sector_s *dest = &sim_data.sectors_flat[next_event->dest_sector_id];
		// If source and dest are neighbours to the local or are the local node then bounce both spheres.
		// If one of them but not the other is a neighbour or the local node then bounce the relevant sphere
		// using the copied data in next_event.
		// This will overwrite the copy which is fine.
		if((source->is_neighbour || SECTOR->id == next_event->source_sector_id) && (dest->is_neighbour || SECTOR->id == next_event->dest_sector_id)){		
			if(source->is_local_neighbour){
				s1 = &next_event->sphere_1; // dummy
			} else {
				s1 = &source->spheres[next_event->sphere_1.sector_id]; // local copy
			}
			if(dest->is_local_neighbour){
				s2 = &next_event->sphere_2; // dummy
			} else {
				s2 = &dest->spheres[next_event->sphere_2.sector_id]; // local copy
			}
			apply_bounce_between_spheres(s1, s2);
			if(source->id == SECTOR->id){
				write_iteration_data(s1, s2);
			}
		} else if((source->is_neighbour && !source->is_local_neighbour) || SECTOR->id == next_event->source_sector_id){
			s1 = &source->spheres[next_event->sphere_1.sector_id];
			s2 = &next_event->sphere_2;
			apply_bounce_between_spheres(s1, s2);
		} else if((dest->is_neighbour && !dest->is_local_neighbour) || SECTOR->id == next_event->dest_sector_id){
			s1 = &next_event->sphere_1;
			s2 = &dest->spheres[next_event->sphere_2.sector_id];
			apply_bounce_between_spheres(s1, s2);
		}
		if(source->id != SECTOR->id){
			seek_two_spheres();
		}
	}
}

// For debugging
// Helps catch any issues with transfering spheres between sectors.
static void sanity_check() {
	int i;
	for(i = 0; i < SECTOR->num_spheres; i++){
		struct sphere_s *sphere = &SECTOR->spheres[i];
		int error = 0;
		enum axis a;
		for (a = X_AXIS; a <= Z_AXIS; a++) {
			if (sphere->pos.vals[a] > SECTOR->end.vals[a]) {
				error = 1;
			}
			if (sphere->pos.vals[a] < SECTOR->start.vals[a]) {
				error = 1;
			}
		}
		if (error) {
			printf("Sector at %d, %d, %d incorrectly has sphere with pos %f, %f, %f\n", SECTOR->pos.x, SECTOR->pos.y, SECTOR->pos.z, sphere->pos.x, sphere->pos.y, sphere->pos.z);
			exit(1);
		}
	}
}

double update_grid() {
	//printf("%d: I have %ld spheres\n", GRID_RANK, SECTOR->num_spheres);
	sanity_check();
	// First reset records.
	reset_event_details();
	// Now find event + time of event
	// Final event may take place after time limit, so cut it short if so
	find_event_times_for_sector(SECTOR);
	reduce_events();
	if (sim_data.time_limit - sim_data.elapsed_time < next_event->time) {
		next_event->time = sim_data.time_limit - sim_data.elapsed_time;
		if(GRID_RANK == 0){
			MPI_Status s;
			MPI_File_write(MPI_OUTPUT_FILE, &sim_data.time_limit, 1, MPI_DOUBLE, &s);
		}
		update_spheres();
	} else {
		update_spheres();
		apply_event();
	}
	//printf("%d: I have %ld spheres\n", GRID_RANK, SECTOR->num_spheres);
	sanity_check();
	return next_event->time;
}

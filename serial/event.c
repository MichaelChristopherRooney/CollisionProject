#include <float.h>
#include <stdlib.h>

#include "collision.h"
#include "event.h"
#include "sector.h"
#include "simulation.h"
#include "sphere.h"

static void set_valid_time_for_all_but_source(){
	int i;
	for(i = 0; i < sim_data.num_sectors; i++){
		struct sector_s *s = &sim_data.sectors_flat[i];
		if(event_details.source_sector->id == s->id){
			s->prior_time_valid = false;
		} else {
			s->prior_time_valid = true;
			sector_events[i].time -= event_details.time;
			//printf("Next valid time is: %f\n", sector_events[i].time);
		}
	}
}

static void set_valid_time_for_all_but_source_and_dest(){
	int i;
	for(i = 0; i < sim_data.num_sectors; i++){
		struct sector_s *s = &sim_data.sectors_flat[i];
		if(event_details.source_sector->id == s->id || event_details.dest_sector->id){
			s->prior_time_valid = false;
		} else {
			s->prior_time_valid = true;
			sector_events[i].time -= event_details.time;
			//printf("Next valid time is: %f\n", sector_events[i].time);
		}
	}
}

void apply_event_dd(){
	if (event_details.type == COL_SPHERE_WITH_GRID) {
		//printf("a\n");
		event_details.sphere_1->vel.vals[event_details.grid_axis] *= -1.0;
		stats.num_grid_collisions++;
		set_valid_time_for_all_but_source();
	} else if (event_details.type == COL_TWO_SPHERES) {
		//printf("b\n");
		apply_bounce_between_spheres(event_details.sphere_1, event_details.sphere_2);
		stats.num_two_sphere_collisions++;
		set_valid_time_for_all_but_source();
	} else if (event_details.type == COL_SPHERE_WITH_SECTOR) {
		//printf("Transfering sphere %d from %d to %d\n", event_details.sphere_1->id, event_details.source_sector->id, event_details.dest_sector->id);
		remove_sphere_from_sector(event_details.source_sector, event_details.sphere_1);
		add_sphere_to_sector(event_details.dest_sector, event_details.sphere_1);
		stats.num_sector_transfers++;
		set_valid_time_for_all_but_source_and_dest();
	} else if(event_details.type == COL_TWO_SPHERES_PARTIAL_CROSSING){
		//printf("d\n");
		apply_bounce_between_spheres(event_details.sphere_1, event_details.sphere_2);
		stats.num_partial_crossings++;
		set_valid_time_for_all_but_source_and_dest();
	}
}

void apply_event_no_dd(){
	if (event_details.type == COL_SPHERE_WITH_GRID) {
		event_details.sphere_1->vel.vals[event_details.grid_axis] *= -1.0;
		stats.num_grid_collisions++;
	} else if (event_details.type == COL_TWO_SPHERES) {
		apply_bounce_between_spheres(event_details.sphere_1, event_details.sphere_2);
		stats.num_two_sphere_collisions++;
	}
}


void reset_event(){
	event_details.time = DBL_MAX;
	event_details.sphere_1 = NULL;
	event_details.sphere_2 = NULL;
	event_details.source_sector = NULL;
	event_details.dest_sector = NULL;
	event_details.type = COL_NONE;
	event_details.grid_axis = AXIS_NONE;
}

void reset_sector_event(int i){
	sector_events[i].time = DBL_MAX;
	sector_events[i].sphere_1 = NULL;
	sector_events[i].sphere_2 = NULL;
	sector_events[i].source_sector = NULL;
	sector_events[i].dest_sector = NULL;
	sector_events[i].type = COL_NONE;
	sector_events[i].grid_axis = AXIS_NONE;
}

static void set_event_details_normal(
	const double time, const enum event_type type, struct sphere_s *sphere_1, 
	struct sphere_s *sphere_2, const enum axis grid_axis, struct sector_s *source_sector,
	struct sector_s *dest_sector
){
	event_details.time = time;
	event_details.type = type;
	event_details.sphere_1 = sphere_1;
	event_details.sphere_2 = sphere_2;
	event_details.grid_axis = grid_axis;
	event_details.source_sector = source_sector;
	event_details.dest_sector = dest_sector;
}

static void set_sector_event_details(
	const double time, const enum event_type type, struct sphere_s *sphere_1, 
	struct sphere_s *sphere_2, const enum axis grid_axis, struct sector_s *source_sector,
	struct sector_s *dest_sector
){
	int i = source_sector->id;
	sector_events[i].time = time;
	sector_events[i].type = type;
	sector_events[i].sphere_1 = sphere_1;
	sector_events[i].sphere_2 = sphere_2;
	sector_events[i].grid_axis = grid_axis;
	sector_events[i].source_sector = source_sector;
	sector_events[i].dest_sector = dest_sector;
}

void set_event_details_from_sector(int id){
	struct event_s *e = &sector_events[id];
	if(e->time < event_details.time){
		set_event_details_normal(e->time, e->type, e->sphere_1, e->sphere_2, e->grid_axis, e->source_sector, e->dest_sector);
	}
}
// Set overall soonest time if needed.
// Also set soonest time for the specific sector.
void set_event_details(
	const double time, const enum event_type type, struct sphere_s *sphere_1, 
	struct sphere_s *sphere_2, const enum axis grid_axis, struct sector_s *source_sector,
	struct sector_s *dest_sector
){
	if(time < event_details.time){
		set_event_details_normal(time, type, sphere_1, sphere_2, grid_axis, source_sector, dest_sector);
	}
	if(sim_data.num_sectors > 1){
		int i = source_sector->id;
		if(time < sector_events[i].time){
			set_sector_event_details(time, type, sphere_1, sphere_2, grid_axis, source_sector, dest_sector);
		}
	}

}


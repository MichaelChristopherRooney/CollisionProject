#include <float.h>
#include <stdlib.h>

#include "collision.h"
#include "event.h"
#include "sector.h"
#include "simulation.h"
#include "sphere.h"

void apply_event(){
	if (event_details.type == COL_SPHERE_WITH_GRID) {
		event_details.sphere_1->vel.vals[event_details.grid_axis] *= -1.0;
		stats.num_grid_collisions++;
	} else if (event_details.type == COL_TWO_SPHERES) {
		apply_bounce_between_spheres(event_details.sphere_1, event_details.sphere_2);
		stats.num_two_sphere_collisions++;
	} else if (event_details.type == COL_SPHERE_WITH_SECTOR) {
		remove_sphere_from_sector(event_details.source_sector, event_details.sphere_1);
		add_sphere_to_sector(event_details.dest_sector, event_details.sphere_1);
		stats.num_sector_transfers++;
	} else if(event_details.type == COL_TWO_SPHERES_PARTIAL_CROSSING){
		apply_bounce_between_spheres(event_details.sphere_1, event_details.sphere_2);
		stats.num_partial_crossings++;
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


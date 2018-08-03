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

void reset_sector_event(struct sector_s *s){
	s->event_details.time = DBL_MAX;
	s->event_details.sphere_1 = NULL;
	s->event_details.sphere_2 = NULL;
	s->event_details.source_sector = NULL;
	s->event_details.dest_sector = NULL;
	s->event_details.type = COL_NONE;
	s->event_details.grid_axis = AXIS_NONE;
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
	source_sector->event_details.time = time;
	source_sector->event_details.type = type;
	source_sector->event_details.sphere_1 = sphere_1;
	source_sector->event_details.sphere_2 = sphere_2;
	source_sector->event_details.grid_axis = grid_axis;
	source_sector->event_details.source_sector = source_sector;
	source_sector->event_details.dest_sector = dest_sector;
}

static void set_event_details_from_sector(struct sector_s *s){
	event_details.time = s->event_details.time;
	event_details.type = s->event_details.type;
	event_details.sphere_1 = s->event_details.sphere_1;
	event_details.sphere_2 = s->event_details.sphere_2;
	event_details.grid_axis = s->event_details.grid_axis;
	event_details.source_sector = s->event_details.source_sector;
	event_details.dest_sector = s->event_details.dest_sector;
}

void find_soonest_event_from_sectors(){
	int soonest = -1;
	double t = DBL_MAX;
	int i;
	for(i = 0; i < sim_data.num_sectors; i++){
		if(sim_data.sectors_flat[i].event_details.time < t){
			t = sim_data.sectors_flat[i].event_details.time;
			soonest = i;
		}
	}
	set_event_details_from_sector(&sim_data.sectors_flat[soonest]);
}

// If domain decomposition is used then set the sector's next event.
// Once all sectors have been checked the actual next event will be determined
// by comparing each sector's own event.
// If no domain decomposition then just set the event normally.
void set_event_details(
	const double time, const enum event_type type, struct sphere_s *sphere_1, 
	struct sphere_s *sphere_2, const enum axis grid_axis, struct sector_s *source_sector,
	struct sector_s *dest_sector
){
	if(sim_data.num_sectors > 1 && time < source_sector->event_details.time){
		set_sector_event_details(time, type, sphere_1, sphere_2, grid_axis, source_sector, dest_sector);
	} else if(sim_data.num_sectors == 1 && time < event_details.time){
		set_event_details_normal(time, type, sphere_1, sphere_2, grid_axis, NULL, NULL);
	}

}


#pragma once

#include <stdbool.h>

#include "sector.h"
#include "sphere.h"

double find_collision_time_spheres(const struct sphere_s *s1, const struct sphere_s *s2);
double find_collision_time_grid(const struct sphere_s *s, enum axis *col_axis);
double find_collision_time_sector(const struct sector_s *sector, const struct sphere_s *sphere, struct sector_s **dest);
void apply_bounce_between_spheres(struct sphere_s *s1, struct sphere_s *s2);
void find_partial_crossing_events_for_all_sectors();
void find_event_times_for_all_sectors();
void find_event_times_no_dd();

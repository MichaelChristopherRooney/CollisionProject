#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "grid.h"
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
	printf("TODO: get adjacent sector non diagonal MPI\n");
	return NULL;
	/*
	int x = sector->pos.x + SECTOR_MODIFIERS[dir][a][X_AXIS];
	int y = sector->pos.y + SECTOR_MODIFIERS[dir][a][Y_AXIS];
	int z = sector->pos.z + SECTOR_MODIFIERS[dir][a][Z_AXIS];
	struct sector_s *adj = &grid->sectors[x][y][z];
	return adj;
	*/
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

// Given a sphere that is known to be heading towards the given sector
// check if the sphere will collide with spheres in the sector.
static void find_partial_crossing_events_between_sphere_and_sector(const struct sphere_s *sphere_1, const struct sector_s *sector_2) {
	int j;
	for (j = 0; j < sector_2->num_spheres; j++) {
		struct sphere_s *sphere_2 = &sector_2->spheres[j];
		double time = find_collision_time_spheres(sphere_1, sphere_2);
		if (time < event_details.time) {
			event_details.type = COL_TWO_SPHERES;
			event_details.sphere_1 = sphere_1;
			event_details.sphere_2 = sphere_2;
			event_details.time = time;
		}
	}
}

// Checks for partial crossings with sectors that are immediately adjacent to 
// the left/right, top/bottom or front/back.
static void find_partial_crossing_events_for_sector_directly_adjacent(const struct sphere_s *sphere, const struct sector_s *sector, const union vector_3d new_pos) {
	printf("TODO: find_partial_crossing_events_for_sector_directly_adjacent MPI\n");
	/*enum axis a;
	for (a = X_AXIS; a <= Z_AXIS; a++) { // right/up/forward on x/y/z axis
		if (sphere->vel.vals[a] >= 0.0 && sector->pos.vals[a] != SECTOR_DIMS[a] - 1) {
			int s_x = sector->pos.x + SECTOR_MODIFIERS[DIR_POSITIVE][a][X_AXIS];
			int s_y = sector->pos.y + SECTOR_MODIFIERS[DIR_POSITIVE][a][Y_AXIS];
			int s_z = sector->pos.z + SECTOR_MODIFIERS[DIR_POSITIVE][a][Z_AXIS];
			struct sector_s *sector_2 = &grid->sectors[s_x][s_y][s_z];
			if (new_pos.vals[a] >= sector_2->start.vals[a] - sphere->radius - sector_2->largest_radius) {
				find_partial_crossing_events_between_sphere_and_sector(sphere, sector_2);
			}
		}
	}
	for (a = X_AXIS; a <= Z_AXIS; a++) { // left/down/behind on x/y/z axis
		if (sphere->vel.vals[a] <= 0.0 && sector->pos.vals[a] != 0) {
			int s_x = sector->pos.x + SECTOR_MODIFIERS[DIR_NEGATIVE][a][X_AXIS];
			int s_y = sector->pos.y + SECTOR_MODIFIERS[DIR_NEGATIVE][a][Y_AXIS];
			int s_z = sector->pos.z + SECTOR_MODIFIERS[DIR_NEGATIVE][a][Z_AXIS];
			struct sector_s *sector_2 = &grid->sectors[s_x][s_y][s_z];
			if (new_pos.vals[a] <= sector_2->end.vals[a] + sphere->radius + sector_2->largest_radius) {
				find_partial_crossing_events_between_sphere_and_sector(sphere, sector_2);
			}
		}
	}*/
}

// Positive in both axes
static void find_partial_crossing_events_for_sector_diagonally_adjacent_pos_pos(const struct sphere_s *sphere, const struct sector_s *sector, const union vector_3d new_pos, enum axis a1, enum axis a2) {
	/*if (sphere->vel.vals[a1] >= 0.0 && sector->pos.vals[a1] != SECTOR_DIMS[a1] - 1 && sphere->vel.vals[a2] >= 0.0 && sector->pos.vals[a2] != SECTOR_DIMS[a2] - 1) {
		int x = sector->pos.x + SECTOR_MODIFIERS[DIR_POSITIVE][a1][X_AXIS] + SECTOR_MODIFIERS[DIR_POSITIVE][a2][X_AXIS];
		int y = sector->pos.y + SECTOR_MODIFIERS[DIR_POSITIVE][a1][Y_AXIS] + SECTOR_MODIFIERS[DIR_POSITIVE][a2][Y_AXIS];
		int z = sector->pos.z + SECTOR_MODIFIERS[DIR_POSITIVE][a1][Z_AXIS] + SECTOR_MODIFIERS[DIR_POSITIVE][a2][Z_AXIS];
		struct sector_s *sector_2 = &grid->sectors[x][y][z];
		if (new_pos.vals[a1] >= sector_2->start.vals[a1] - sphere->radius - sector_2->largest_radius && new_pos.vals[a2] >= sector_2->start.vals[a2] - sphere->radius - sector_2->largest_radius) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector_2);
		}
	}*/
}

// Positive in a1 and negative in a2
static void find_partial_crossing_events_for_sector_diagonally_adjacent_pos_neg(const struct sphere_s *sphere, const struct sector_s *sector, const union vector_3d new_pos, enum axis a1, enum axis a2) {
	/*if (sphere->vel.vals[a1] >= 0.0 && sector->pos.vals[a1] != SECTOR_DIMS[a1] - 1 && sphere->vel.vals[a2] <= 0.0 && sector->pos.vals[a2] != 0) {
		int x = sector->pos.x + SECTOR_MODIFIERS[DIR_POSITIVE][a1][X_AXIS] + SECTOR_MODIFIERS[DIR_NEGATIVE][a2][X_AXIS];
		int y = sector->pos.y + SECTOR_MODIFIERS[DIR_POSITIVE][a1][Y_AXIS] + SECTOR_MODIFIERS[DIR_NEGATIVE][a2][Y_AXIS];
		int z = sector->pos.z + SECTOR_MODIFIERS[DIR_POSITIVE][a1][Z_AXIS] + SECTOR_MODIFIERS[DIR_NEGATIVE][a2][Z_AXIS];
		struct sector_s *sector_2 = &grid->sectors[x][y][z];
		if (new_pos.vals[a1] >= sector_2->start.vals[a1] - sphere->radius - sector_2->largest_radius && new_pos.vals[a2] <= sector_2->end.vals[a2] + sphere->radius + sector_2->largest_radius) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector_2);
		}
	}*/
}

// Negative in a1 and positive in a2
static void find_partial_crossing_events_for_sector_diagonally_adjacent_neg_pos(const struct sphere_s *sphere, const struct sector_s *sector, const union vector_3d new_pos, enum axis a1, enum axis a2) {
	/*if (sphere->vel.vals[a1] <= 0.0 && sector->pos.vals[a1] != 0 && sphere->vel.vals[a2] >= 0.0 && sector->pos.vals[a2] != SECTOR_DIMS[a2] - 1) {
		int x = sector->pos.x + SECTOR_MODIFIERS[DIR_NEGATIVE][a1][X_AXIS] + SECTOR_MODIFIERS[DIR_POSITIVE][a2][X_AXIS];
		int y = sector->pos.y + SECTOR_MODIFIERS[DIR_NEGATIVE][a1][Y_AXIS] + SECTOR_MODIFIERS[DIR_POSITIVE][a2][Y_AXIS];
		int z = sector->pos.z + SECTOR_MODIFIERS[DIR_NEGATIVE][a1][Z_AXIS] + SECTOR_MODIFIERS[DIR_POSITIVE][a2][Z_AXIS];
		struct sector_s *sector_2 = &grid->sectors[x][y][z];
		if (new_pos.vals[a1] <= sector_2->end.vals[a1] + sphere->radius + sector_2->largest_radius && new_pos.vals[a2] >= sector_2->start.vals[a2] - sphere->radius - sector_2->largest_radius) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector_2);
		}
	}*/
}

// Negative in both axes
static void find_partial_crossing_events_for_sector_diagonally_adjacent_neg_neg(const struct sphere_s *sphere, const struct sector_s *sector, const union vector_3d new_pos, enum axis a1, enum axis a2) {
	/*if (sphere->vel.vals[a1] <= 0.0 && sector->pos.vals[a1] != 0 && sphere->vel.vals[a2] <= 0.0 && sector->pos.vals[a2] != 0) {
		int x = sector->pos.x + SECTOR_MODIFIERS[DIR_NEGATIVE][a1][X_AXIS] + SECTOR_MODIFIERS[DIR_NEGATIVE][a2][X_AXIS];
		int y = sector->pos.y + SECTOR_MODIFIERS[DIR_NEGATIVE][a1][Y_AXIS] + SECTOR_MODIFIERS[DIR_NEGATIVE][a2][Y_AXIS];
		int z = sector->pos.z + SECTOR_MODIFIERS[DIR_NEGATIVE][a1][Z_AXIS] + SECTOR_MODIFIERS[DIR_NEGATIVE][a2][Z_AXIS];
		struct sector_s *sector_2 = &grid->sectors[x][y][z];
		if (new_pos.vals[a1] <= sector_2->end.vals[a1] + sphere->radius + sector_2->largest_radius && new_pos.vals[a2] >= sector_2->end.vals[a2] + sphere->radius + sector_2->largest_radius) {
			find_partial_crossing_events_between_sphere_and_sector(sphere, sector_2);
		}
	}*/
}

// Checks for partial crossings with sectors that are diagonally adjacent along two axes.
static void find_partial_crossing_events_for_sector_diagonally_adjacent(const struct sphere_s *sphere, const struct sector_s *sector, const union vector_3d new_pos) {
	printf("TODO: find_partial_crossing_events_for_sector_diagonally_adjacent MPI\n");
	/*
	// x/y combinations
	if (grid->xy_check_needed) {
		find_partial_crossing_events_for_sector_diagonally_adjacent_pos_pos(sphere, sector, new_pos, X_AXIS, Y_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_pos_neg(sphere, sector, new_pos, X_AXIS, Y_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_neg_pos(sphere, sector, new_pos, X_AXIS, Y_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_neg_neg(sphere, sector, new_pos, X_AXIS, Y_AXIS);
	}
	// x/z combinations
	if (grid->xz_check_needed) {
		find_partial_crossing_events_for_sector_diagonally_adjacent_pos_pos(sphere, sector, new_pos, X_AXIS, Z_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_pos_neg(sphere, sector, new_pos, X_AXIS, Z_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_neg_pos(sphere, sector, new_pos, X_AXIS, Z_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_neg_neg(sphere, sector, new_pos, X_AXIS, Z_AXIS);
	}
	// y/z combinations
	if (grid->yz_check_needed) {
		find_partial_crossing_events_for_sector_diagonally_adjacent_pos_pos(sphere, sector, new_pos, Y_AXIS, Z_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_pos_neg(sphere, sector, new_pos, Y_AXIS, Z_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_neg_pos(sphere, sector, new_pos, Y_AXIS, Z_AXIS);
		find_partial_crossing_events_for_sector_diagonally_adjacent_neg_neg(sphere, sector, new_pos, Y_AXIS, Z_AXIS);
	}*/
}

// For each sphere check which sectors it is going towards.
// If by the time of the current soonest event it is within a certain distance 
// of any sector it is travelling towards we must check for partial crossings.
void find_partial_crossing_events_for_sector(const struct sector_s *sector) {
	int i;
	for (i = 0; i < sector->num_spheres; i++) {
		const struct sphere_s *sphere = &sector->spheres[i];
		const union vector_3d new_pos = {
			.x = sphere->pos.x + (sphere->vel.x * event_details.time),
			.y = sphere->pos.y + (sphere->vel.y * event_details.time),
			.z = sphere->pos.z + (sphere->vel.z * event_details.time)
		};
		find_partial_crossing_events_for_sector_directly_adjacent(sphere, sector, new_pos);
		find_partial_crossing_events_for_sector_diagonally_adjacent(sphere, sector, new_pos);
		// TODO: 3d diagonal
	}
}

// Finds when all spheres in a given sector will collide with other and returns
// the soonest time.
static void find_collision_times_between_spheres_in_sector(const struct sector_s *sector) {
	int i, j;
	for (i = 0; i < sector->num_spheres - 1; i++) {
		struct sphere_s *s1 = &sector->spheres[i];
		for (j = i + 1; j < sector->num_spheres; j++) {
			struct sphere_s *s2 = &sector->spheres[j];
			double time = find_collision_time_spheres(s1, s2);
			if (time < event_details.time) {
				event_details.type = COL_TWO_SPHERES;
				event_details.sphere_1 = s1;
				event_details.sphere_2 = s2;
				event_details.time = time;
			}
		}
	}
}

// Finds time to both collide with grid and to cross sector boundaries.
// Can optimise further so that grid boundaries are not checked if another
// sector will be entered first.
static void find_collision_times_grid_boundary_for_sector(const struct sector_s *sector) {
	enum axis axis = COL_NONE;
	int i;
	for (i = 0; i < sector->num_spheres; i++) {
		struct sphere_s *sphere = &sector->spheres[i];
		double time = find_collision_time_grid(sphere, &axis);
		if (time < event_details.time) {
			event_details.type = COL_SPHERE_WITH_GRID;
			event_details.sphere_1 = sphere;
			event_details.time = time;
			event_details.grid_axis = axis;
		}
		struct sector_s *temp_dest;
		time = find_collision_time_sector(sector, sphere, &temp_dest);
		if (time < event_details.time) {
			event_details.type = COL_SPHERE_WITH_SECTOR;
			event_details.sphere_1 = sphere;
			event_details.time = time;
			event_details.source_sector = sector;
			event_details.dest_sector = temp_dest;
		}
	}
}

// Given a sector finds the soonest occuring event.
// The event will be either two spheres colliding, a sphere colliding with a grid
// boundary, or a sphere passing into another sector.
// Any partial crossings will be handled once this function has been called for each sector.
void find_event_times_for_sector(const struct sector_s *sector) {
	if (sector->num_spheres == 0) {
		return;
	}
	find_collision_times_between_spheres_in_sector(sector);
	find_collision_times_grid_boundary_for_sector(sector);
}

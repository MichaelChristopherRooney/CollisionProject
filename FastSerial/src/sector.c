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
		sector->spheres = realloc(sector->spheres, sector->max_spheres * sizeof(struct sphere_s *));
	}
	sector->spheres[sector->num_spheres] = sphere;
	sector->spheres[sector->num_spheres]->sector_id = sector->num_spheres;
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
	int i;
	for (i = sphere->sector_id; i < sector->num_spheres - 1; i++) {
		sector->spheres[i] = sector->spheres[i + 1];
		sector->spheres[i]->sector_id--;
	}
}

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
	struct sector_s *adj = &grid->sectors[x][y][z];
	return adj;
}

// Helper for add_sphere_to_correct_sector function.
static bool does_sphere_belong_to_sector(const struct sphere_s *sphere, const struct sector_s *sector) {
	enum axis a;
	for (a = X_AXIS; a <= Z_AXIS; a++) {
		if (sphere->pos.vals[a] < sector->start.vals[a] || sphere->pos.vals[a] >= sector->end.vals[a]) {
			return false;
		}
	}
	return true;
}

// Given a sphere adds it to the correct sector.
// This should only be used when randomly generating spheres at startup.
void add_sphere_to_correct_sector(const struct sphere_s *sphere) {
	int x, y, z;
	for (x = 0; x < SECTOR_DIMS[X_AXIS]; x++) {
		for (y = 0; y < SECTOR_DIMS[Y_AXIS]; y++) {
			for (z = 0; z < SECTOR_DIMS[Z_AXIS]; z++) {
				bool res = does_sphere_belong_to_sector(sphere, &grid->sectors[x][y][z]);
				if (res) {
					add_sphere_to_sector(&grid->sectors[x][y][z], sphere);
					return;
				}
			}
		}
	}
	// Shouldn't reach here
	printf("Error: sphere does not belong to any sector\n");
	getchar();
	exit(1);
}
#include <stdio.h>
#include <stdlib.h>

#include "grid.h"
#include "sector.h"

// Can be indexed using coord enum
const int SECTOR_DIMS[3] = { NUM_SECTORS_X, NUM_SECTORS_Y, NUM_SECTORS_Z };

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
	if (sector->head == NULL) {
		sector->head = calloc(1, sizeof(struct sphere_list_s));
		sector->head->sphere = sphere;
		sector->num_spheres = 1;
		sector->largest_radius = sphere->radius;
		return;
	}
	struct sphere_list_s *cur = sector->head;
	while (cur->next != NULL) {
		cur = cur->next;
	}
	cur->next = calloc(1, sizeof(struct sphere_list_s));
	cur->next->sphere = sphere;
	cur->next->prev = cur;
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

void remove_sphere_from_sector(struct sector_s *sector, const struct sphere_s *sphere) {
	struct sphere_list_s *cur = sector->head;
	while (cur->sphere != sphere) {
		cur = cur->next;
	}
	if (cur == sector->head && cur->next != NULL) {
		sector->head = cur->next;
		cur->next->prev = NULL;
	} else if (cur == sector->head && cur->next == NULL) {
		sector->head = NULL;
	} else if (cur->next != NULL) {
		cur->prev->next = cur->next;
		cur->next->prev = cur->prev;
	} else if (cur->next == NULL) {
		cur->prev->next = NULL;
	}
	free(cur);
	sector->num_spheres--;
	set_largest_radius_after_removal(sector, sphere);
}

// Given a sector returns the sector adjacent diagonally in the specified
// direction on both axes.
// Ex: if provided sector is at { 0, 0, 0 } and the sector adjacent positively
// along the x and y axis is requested then the sector at { 1, 1, 0 } will be returned.
struct sector_s *get_adjacent_sector_diagonal(const struct sector_s *sector, const enum coord c1, const enum direction c1_dir, const enum coord c2, const enum direction c2_dir) {
	int x = sector->pos.x + SECTOR_MODIFIERS[c1_dir][c1][X_COORD] + SECTOR_MODIFIERS[c2_dir][c2][X_COORD];
	int y = sector->pos.y + SECTOR_MODIFIERS[c1_dir][c1][Y_COORD] + SECTOR_MODIFIERS[c2_dir][c2][Y_COORD];
	int z = sector->pos.z + SECTOR_MODIFIERS[c1_dir][c1][Z_COORD] + SECTOR_MODIFIERS[c2_dir][c2][Z_COORD];
	struct sector_s *adj = &grid->sectors[x][y][z];
	return adj;
}

// Given a sector returns the sector adjacent in specified direction on the
// specified axis.
// Ex: If x axis and positive direction returns sector to the right.
struct sector_s *get_adjacent_sector_non_diagonal(const struct sector_s *sector, const enum coord c, const enum direction dir) {
	int x = sector->pos.x + SECTOR_MODIFIERS[dir][c][X_COORD];
	int y = sector->pos.y + SECTOR_MODIFIERS[dir][c][Y_COORD];
	int z = sector->pos.z + SECTOR_MODIFIERS[dir][c][Z_COORD];
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
	for (x = 0; x < NUM_SECTORS_X; x++) {
		for (y = 0; y < NUM_SECTORS_Y; y++) {
			for (z = 0; z < NUM_SECTORS_Z; z++) {
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
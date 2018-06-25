#include <stdio.h>
#include <stdlib.h>

#include "sector.h"

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

// Note: when removing a sphere that has the largest radius we need to 
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
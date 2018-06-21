#include <stdlib.h>

#include "sector.h"

void add_sphere_to_sector(struct sector_s *sector, struct sphere_s *sphere) {
	if (sector->head == NULL) {
		sector->head = calloc(1, sizeof(struct sphere_list_s));
		sector->head->sphere = sphere;
		sector->num_spheres = 1;
		return;
	}
	struct sphere_list_s *cur = sector->head;
	while (cur->next != NULL) {
		cur = cur->next;
	}
	cur->next = calloc(1, sizeof(struct sphere_list_s));
	cur->next->sphere = sphere;
	sector->num_spheres++;
}
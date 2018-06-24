#include <stdlib.h>

#include "sector.h"

void add_sphere_to_sector(struct sector_s *sector, const struct sphere_s *sphere) {
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
	cur->next->prev = cur;
	sector->num_spheres++;
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
}
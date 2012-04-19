#include "pair.h"

#include <stdlib.h>

pair *create_pair(void *a, void *b) {
	pair *p = malloc(sizeof(pair));
	p->a = a;
	p->b = b;
	return p;
}
void destory_pair(pair *p, int free_data) {
	if (free_data) {
		free(p->a);
		free(p->b);
	}
	free(p);
}

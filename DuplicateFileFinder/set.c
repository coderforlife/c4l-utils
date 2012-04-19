#include "set.h"

#include <stdlib.h>

set *set_create(compare_func cmp) {
	return map_create(cmp);
}

int set_traverse(set *s, int method, traverse_cb cb, void *param) {
	return map_traverse(s, method, cb, param);
}

void *set_contains(set *s, void *obj) {
	pair *p = map_get(s, obj);
	return p ? p->a : NULL;
}

void *set_insert(set *s, void *obj) {
	pair *p = map_set(s, obj, (void*)1);
	return p ? p->a : NULL;
}

void *set_delete(set *s, void *obj) {
	pair *p = map_remove(s, obj);
	return p ? p->a : NULL;
}

void set_clear(set *s, int free_data) {
	map_clear(s, free_data);
}

void set_destroy(set *s, int free_data) {
	map_destroy(s, free_data);
}

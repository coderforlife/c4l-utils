// A simple map for C
// Uses an AVL tree

#ifndef _MAP_H_
#define _MAP_H_

#include "pair.h"

#define PREORDER	-1
#define INORDER		0
#define POSTORDER	1

typedef int(*traverse_cb)(void *key, void *value, void *param); // return non-0 to stop traversal, otherwise continue

typedef int(*compare_func)(const void*,const void*);

typedef struct _map_node {
	union {
		struct { struct _map_node *left, *right; };
		struct _map_node *children[2];
	};
	pair *x;
	int balance;
} map_node;

typedef struct _map {
	map_node *x;
	compare_func _cmp;
	unsigned int size;
} map;

map *map_create(compare_func cmp);
int map_traverse(map *m, int method, traverse_cb cb, void *param);	// O(n)	recursive
pair*map_get    (map *m, void *key);		// O(log(n))	non-recursive!
pair*map_set    (map *m, void *key, void *value);		// O(log(n))	non-recursive!
pair*map_remove (map *m, void *key);		// O(log(n))	non-recursive!
void map_clear  (map *m, int free_data);	// O(n)			recursive
void map_destroy(map *m, int free_data);	// O(n)			recursive

#endif

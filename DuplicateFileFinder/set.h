// A simple set for C
// Uses an AVL tree

#ifndef _SET_H_
#define _SET_H_

#include "map.h"

typedef map set;

set *set_create(compare_func cmp);
int  set_traverse(set *s, int method, traverse_cb cb, void *param);	// O(n)	recursive
void*set_contains(set *s, void *obj);		// O(log(n))	non-recursive!
void*set_insert  (set *s, void *obj);		// O(log(n))	non-recursive!
void*set_delete  (set *s, void *obj);		// O(log(n))	non-recursive!
void set_clear   (set *s, int free_data);	// O(n)			recursive
void set_destroy (set *s, int free_data);	// O(n)			recursive

#endif

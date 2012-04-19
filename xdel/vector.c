#include "vector.h"

#include <stdlib.h>
#include <string.h>

vector *vector_create(unsigned int capacity) {
	vector *v = malloc(sizeof(vector));
	v->x = malloc(capacity*sizeof(void*));
	v->length = 0;
	v->_capacity = capacity;
	return v;
}

void *vector_append(vector *v, void *obj) {
	return vector_insert(v, v->length, obj);
}

void *vector_insert(vector *v, unsigned int ind, void *obj) {
	if (ind > v->length) return NULL;
	if (v->length >= v->_capacity) {
		v->_capacity *= 2;
		v->x = realloc(v->x, v->_capacity*sizeof(void*));
	}
	if (ind < v->length) {
		memmove(v->x+ind+1, v->x+ind, sizeof(void*)*(v->length-ind));
	}
	v->x[ind] = obj;
	++v->length;
	return obj;
}

void *vector_delete(vector *v, unsigned int ind) {
	void *obj;
	if (ind >= v->length) return NULL;
	obj = v->x[ind];
	v->x[ind] = NULL;
	if (ind < v->length-1) {
		memmove(v->x+ind, v->x+ind+1, sizeof(void*)*(v->length-ind-1));
	}
	--v->length;
	return obj;
}

void vector_clear(vector *v, unsigned int free_data) {
	if (free_data) {
		unsigned int i;
		for (i = 0; i < v->length; ++i)
			free(v->x[i]);
	}
	//memset(v->x, 0, sizeof(void*)*v->length);
	v->length = 0;
}

void vector_destroy(vector *v, unsigned int free_data) {
	if (free_data) {
		unsigned int i;
		for (i = 0; i < v->length; ++i)
			free(v->x[i]);
	}
	free(v->x);
	free(v);
}

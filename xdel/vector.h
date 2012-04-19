// A simple vector for C

#ifndef _VECTOR_H_
#define _VECTOR_H_

typedef struct _vector {
	void **x;
	unsigned int length, _capacity;
} vector;

vector *vector_create(unsigned int capacity);
void*vector_append (vector *v, void *obj);						// amortized O(1) time
void*vector_insert (vector *v, unsigned int ind, void *obj);	// amortized O(n-ind) time
void*vector_delete (vector *v, unsigned int ind);				// O(n-ind) time
void vector_clear  (vector *v, unsigned int free_data);			// O(1) time when free_data = 0, otherwise O(n)
void vector_destroy(vector *v, unsigned int free_data);			// O(1) time when free_data = 0, otherwise O(n)

#endif

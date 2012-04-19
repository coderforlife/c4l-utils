// A simple pair for C

#ifndef _PAIR_H_
#define _PAIR_H_

typedef struct _pair {
	void *a;
	void *b;
} pair;

pair *create_pair(void *a, void *b);
void destory_pair(pair *p, int free_data);

#endif

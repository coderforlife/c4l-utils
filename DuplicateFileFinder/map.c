#include "map.h"

#include <stdlib.h>

#ifndef MAX_HEIGHT
#define MAX_HEIGHT 32
#endif

map *map_create(compare_func cmp) {
	map *m = malloc(sizeof(map));
	m->_cmp = cmp;
	m->x = NULL;
	m->size = 0;
	return m;
}

static int preorder(map_node *n, traverse_cb cb, void *param) {
	if (n) {
		int x;
		if ((x = cb(n->x->a, n->x->b, param)))
			return x;
		if ((x = preorder(n->left, cb, param)))
			return x;
		if ((x = preorder(n->right, cb, param)))
			return x;
	}
	return 0;
}

static int inorder(map_node *n, traverse_cb cb, void *param) {
	if (n) {
		int x;
		if ((x = inorder(n->left, cb, param)))
			return x;
		if ((x = cb(n->x->a, n->x->b, param)))
			return x;
		if ((x = inorder(n->right, cb, param)))
			return x;
	}
	return 0;
}

static int postorder(map_node *n, traverse_cb cb, void *param) {
	if (n) {
		int x;
		if ((x = postorder(n->left, cb, param)))
			return x;
		if ((x = postorder(n->right, cb, param)))
			return x;
		if ((x = cb(n->x->a, n->x->b, param)))
			return x;
	}
	return 0;
}

int map_traverse(map *m, int method, traverse_cb cb, void *param) {
	if (method == PREORDER)
		return preorder(m->x, cb, param);
	else if (method == INORDER)
		return inorder(m->x, cb, param);
	else if (method == POSTORDER)
		return postorder(m->x, cb, param);
	else
		return 0;
}

map_node *map_find(map *m, void *key) {
	map_node *n = m->x;
	while (n) {
		int c = m->_cmp(key, n->x->a);
		if (c == 0)
			return n;
		else if (c < 0)
			n = n->left;
		else /*if (c > 0)*/
			n = n->right;
	}
	return NULL;
}

pair *map_get(map *m, void *key) {
	map_node *n = map_find(m, key);
	return n ? n->x : NULL;
}

static map_node *rotate_a(map_node *x, map_node *y) {
	map_node *w = x->right;
	x->right = w->left;
	w->left = x;
	y->left = w->right;
	w->right = y;
	if (w->balance == -1) {
		x->balance = 0;
		y->balance = +1;
	} else if (w->balance == 0) {
		x->balance = y->balance = 0;
	} else /* if (w->balance == +1) */ {
		x->balance = -1;
		y->balance = 0;
	}
	w->balance = 0;
	return w;
}

static map_node *rotate_b(map_node *x, map_node *y) {
	map_node *w = x->left;
	x->left = w->right;
	w->right = x;
	y->right = w->left;
	w->left = y;
	if (w->balance == +1) {
		x->balance = 0;
		y->balance = -1;
	} else if (w->balance == 0) {
		x->balance = y->balance = 0;
	} else /* if (w->balance == -1) */ {
		x->balance = +1;
		y->balance = 0;
	}
	w->balance = 0;
	return w;
}

static map_node *rotate_c(map_node *x, map_node *y) {
	map_node *w = x;
	y->left = x->right;
	x->right = y;
	x->balance = y->balance = 0;
	return w;
}

static map_node *rotate_d(map_node *x, map_node *y) {
	map_node *w = x;
	y->right = x->left;
	x->left = y;
	x->balance = y->balance = 0;
	return w;
}

pair *map_set(map *m, void *key, void *value) {
	map_node *y, *z; /* Top node to update balance factor, and parent. */
	map_node *p, *q; /* Iterator, and parent. */
	map_node *n;     /* Newly inserted node. */
	map_node *w;     /* New root of rebalanced subtree. */
	unsigned char dir = 0;			/* Direction to descend. */
	unsigned char ds[MAX_HEIGHT];	/* Cached comparison results. */
	int i = 0;						/* Number of cached results. */

	z = (map_node*)&m->x;
	y = m->x;
	for (q = z, p = y; p != NULL; q = p, p = p->children[dir]) {
		int c = m->_cmp(key, p->x->a);
		if (c == 0)
			return p->x;
		if (p->balance != 0) {
			z = q;
			y = p;
			i = 0;
		}
		ds[i++] = dir = c > 0;
	}

	n = q->children[dir] = malloc(sizeof(map_node));
	if (n == NULL)
		return NULL;

	m->size++;
	n->x = create_pair(key, value);
	n->left = n->right = NULL;
	n->balance = 0;
	if (y == NULL)
		return n->x;

	for (p = y, i = 0; p != n; p = p->children[ds[i]], i++) {
		if (ds[i] == 0)
			p->balance--;
		else
			p->balance++;
	}

	if (y->balance == -2) {
		w = (y->left->balance == -1) ? rotate_c(y->left, y) : rotate_a(y->left, y);
	} else if (y->balance == +2) {
		w = (y->right->balance == +1) ? rotate_d(y->right, y) : rotate_b(y->right, y);
	} else
		return n->x;
	z->children[y != z->left] = w;
	return n->x;
}

pair *map_remove(map *m, void *key) {
	map_node *ns[MAX_HEIGHT];     /* Stack of nodes. */
	unsigned char ds[MAX_HEIGHT]; /* Direction of going down the tree (0 == left, 1 == right). */
	unsigned int i = 0;           /* Stack pointer. */
	map_node *n = m->x;           /* Traverses tree to find node to delete. */
	int c;                        /* Comparison value */
	pair *p;

	if (n == NULL)
		return NULL;

	for (c = -1; c != 0; c = m->_cmp(key, n->x->a)) {
		unsigned char dir = c > 0;
		ns[i] = n;
		ds[i++] = dir;
		if ((n = n->children[dir]) == NULL)
			return NULL;
	}

	p = n->x;

	if (n->right == NULL) {
		ns[i-1]->children[ds[i-1]] = n->left;
	} else {
		map_node *a = n->right;
		if (a->left == NULL) {
			a->left = n->left;
			a->balance = n->balance;
			ns[i-1]->children[ds[i-1]] = a;
			ds[i] = 1;
			ns[i++] = a;
		} else {
			map_node *b;
			int j = i++;
			for (;;) {
				ds[i] = 0;
				ns[i++] = a;
				b = a->left;
				if (b->left == NULL)
					break;
				a = b;
			}
			b->left = n->left;
			a->left = b->right;
			b->right = n->right;
			b->balance = n->balance;
			ns[j-1]->children[ds[j-1]] = b;
			ds[j] = 1;
			ns[j] = b;
		}
	}
	free(n);
	while (--i > 0) {
		map_node *y = ns[i];
		if (ds[i] == 0) {
			y->balance++;
			if (y->balance == +1)
				break;
			else if (y->balance == +2) {
				map_node *x = y->right;
				if (x->balance == -1) {
					ns[i-1]->children[ds[i-1]] = rotate_b(x, y);
				} else {
					y->right = x->left;
					x->left = y;
					ns[i-1]->children[ds[i-1]] = x;
					if (x->balance == 0) {
						x->balance = -1;
						y->balance = +1;
						break;
					} else
						x->balance = y->balance = 0;
				}
			}
		} else {
			y->balance--;
			if (y->balance == -1)
				break;
			else if (y->balance == -2) {
				map_node *x = y->left;
				if (x->balance == +1) {
					ns[i-1]->children[ds[i-1]] = rotate_a(x, y);
				} else {
					y->left = x->right;
					x->right = y;
					ns[i-1]->children[ds[i-1]] = x;
					if (x->balance == 0) {
						x->balance = +1;
						y->balance = -1;
						break;
					} else
						x->balance = y->balance = 0;
				}
			}
		}
	}
	m->size--;
	return p;
}

void map_node_destroy(map_node *n, int free_data) {
	if (n) {
		map_node_destroy(n->left, free_data);
		map_node_destroy(n->right, free_data);
		if (free_data) {
			free(n->x->a);
			if (n->x->b != (void*)1) // for sets
				free(n->x->b);
		}
		free(n->x);
		free(n);
	}
}

void map_clear(map *m, int free_data) {
	map_node_destroy(m->x, free_data);
	m->x = NULL;
}

void map_destroy(map *m, int free_data) {
	map_node_destroy(m->x, free_data);
	free(m);
}

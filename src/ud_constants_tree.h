#ifndef UD_CONSTANTS_TREE_H
#define UD_CONSTANTS_TREE_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "definitions.h"

typedef struct _ud_constant_node {
	struct _ud_constant_node *next;
	key_constant_pair pair;
} udc_node;

udc_node *udc_node_create(const char* term, _double_t value);

void udctree_add(udc_node *node);
void udctree_delete();
size_t udctree_get_num_nodes();
udc_node *udctree_search(const char* term);
udc_node *udctree_get_root();
key_constant_pair *udctree_match(const char* term);

#endif

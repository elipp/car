#ifndef UD_CONSTANTS_TREE_H
#define UD_CONSTANTS_TREE_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "definitions.h"

void udctree_add(udc_node *node);
key_constant_pair *udctree_get(int index);
void udctree_delete();
size_t udctree_get_num_nodes();
udc_node *udctree_search(const char* term);

#endif

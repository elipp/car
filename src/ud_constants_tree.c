#include "ud_constants_tree.h"
#include "definitions.h"

// not implementing udc_tree as a struct, since there will only ever be 1 of them
// Instead, it is implemented as a collection of global variables :-D

static udc_node *udctree_head;
static udc_node *udctree_root;
static size_t udctree_num_nodes;

static void udctree_init() {
	udctree_head = NULL;
	udctree_root = NULL;
	udctree_num_nodes = 0;
}

void udctree_add(udc_node *node) {
	
	if (udctree_num_nodes == 0) {
		udctree_init();
		udctree_head = node;
		udctree_root = node;
		udctree_head->next = NULL;
	}
	else {
		udctree_head->next = node;
		udctree_head = node;
		node->next = NULL;
	}
	++udctree_num_nodes;

}


void udctree_delete() {

	if (udctree_num_nodes > 0) {
		udc_node *iter = udctree_get_root();
		udc_node *nexttmp = iter->next;
		while (nexttmp != NULL) {
			free(iter->pair.key);
			free(iter);
			iter = nexttmp;
			nexttmp = nexttmp->next;
		}
		free(iter->pair.key);
		free(iter);
	}
}

size_t udctree_get_num_nodes() { return udctree_num_nodes; }

udc_node *udctree_search(const char* term) {
	if (udctree_num_nodes == 0) { return NULL; }
	udc_node *iter = udctree_get_root();
	while (iter != NULL) {
		if (strcmp(term, iter->pair.key) == 0) { return iter; }
		iter = iter->next;
	}
	return NULL;
	
}

udc_node *udc_node_create(const char* term, _double_t value) {
	udc_node *node = malloc(sizeof(udc_node));
	node->pair.key = strdup(term);
	node->pair.value = value;
	node->next = NULL;
	return node;
}

udc_node *udctree_get_root() { return udctree_root; }


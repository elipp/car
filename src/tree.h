#ifndef TREE_H
#define TREE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "definitions.h"
#include "utils.h"

typedef struct {
	char* string;
	size_t length;
	int param;	// can represent the sign of or the operation associated with the term
	int requires_parsing;
} term_t;

typedef struct {
	int current_index;
	term_t *terms;
	size_t num_terms;
	int priority_level;
} tree_t;


void tree_add(tree_t *tree, char* term, int param, int requires_parsing);
void tree_delete(tree_t *tree);
tree_t *tree_generate(const char* input, size_t input_length, int priority_level);
_double_t tree_get_result(tree_t *tree);
void tree_dump(tree_t *tree);

#endif

#include "tree.h"

#ifdef LONG_DOUBLE_PRECISION
_double_t (*pow_ptr) (_double_t, _double_t) = powl;
#else
_double_t (*pow_ptr) (_double_t, _double_t) = pow;
#endif

#define OP_NOP 0x0
#define OP_ADD '+'
#define OP_SUB '-'
#define OP_MUL '*'
#define OP_DIV '/'
#define OP_POW '^'
#define OP_MOD '%'

#define PARAM_FIRST 0xF
#define PARAM_NOP 0x0
#define PARAM_SGN_POS 0x1
#define PARAM_SGN_NEG 0x2
#define PARAM_MUL 0x3
#define PARAM_DIV 0x4
#define PARAM_POW 0x5
#define PARAM_MOD 0x6
#define PARAM_FAC 0x7


// see tree_generate
#define MATCHES_WITH_OPERATOR(x) (x == operators[0] || x == operators[1])

// tree constructor
static tree_t *tree_create(size_t num_terms, int priority_level) {
	tree_t *tree = malloc(sizeof(tree_t));
	tree->current_index = 0;
	tree->terms = malloc(num_terms*sizeof(term_t));
	tree->num_terms = num_terms;
	tree->priority_level = priority_level;
	return tree;
}

void tree_add(tree_t *tree, char* term, size_t length, int param, int requires_parsing) {

	tree->terms[tree->current_index].string = term;
	tree->terms[tree->current_index].length = length;
	tree->terms[tree->current_index].param = param;
	tree->terms[tree->current_index].requires_parsing = requires_parsing;

	++tree->current_index;
}

void tree_delete(tree_t *tree) {

	int i = 0;
	for(; i < tree->num_terms; i++) {
		free(tree->terms[i].string);
	}

	free(tree->terms);
	free(tree);

}

tree_t *tree_generate(const char* input, size_t input_length, int priority_level) {

	char operators[2]; int params[2];

		switch(priority_level) {
			case PRIO_ADD_SUB:
				operators[0] = OP_ADD;
				operators[1] = OP_SUB;
				params[0] = PARAM_SGN_POS;
				params[1] = PARAM_SGN_NEG;
				break;
			case PRIO_MUL_DIV:
				operators[0] = OP_MUL;
				operators[1] = OP_DIV;
				params[0] = PARAM_MUL;
				params[1] = PARAM_DIV;
				break;
			case PRIO_POW_FAC:
				operators[0] = OP_POW;
				operators[1] = OP_MOD;	// try with OP_MOD
				params[0] = PARAM_POW;
				params[1] = PARAM_MOD;
				break;
			default:
				operators[0] = OP_NOP;
				operators[1] = OP_NOP;
				params[0] = PARAM_NOP;
				params[1] = PARAM_NOP;
		}

		int i = 0;
		size_t tree_size = 1;
		int num_brackets = 0;

		// count brackets

		for (i = 0; i < input_length; i++) {
				
			if (input[i] == '(') ++num_brackets;
			if (input[i] == ')') --num_brackets;

			if (MATCHES_WITH_OPERATOR(input[i]) && num_brackets == 0) {
				++tree_size;
			}

		}

		if (num_brackets != 0) {
			fprintf(stderr, "Syntax error: unmatched parenthesis!\n");
			return NULL;
		}

		tree_t *tree = tree_create(tree_size, priority_level);


		// find only TOP level terms

		if (tree_size > 1) {

			num_brackets = 0;
			int beg_pos = 0;
			int end_pos;
			i = 0;
			int prev_parm = PARAM_FIRST;

			while(i < input_length) {

				// the contents in between braces are left as is
				
				if (input[i] == '(' && num_brackets == 0) { 
					while (i < input_length) {
						if (input[i] == '(') { ++num_brackets; }
						else
						if (input[i] == ')') {
							--num_brackets;
							if (num_brackets == 0) {
								break;
							}
						}
						++i;
					}



				}

				else if (MATCHES_WITH_OPERATOR(input[i]) && num_brackets == 0) {

					end_pos = i;
					while (input[end_pos] == ' ' && end_pos >= 0) --end_pos;
					const size_t term_len = end_pos-beg_pos;
					char* term = substring(input, beg_pos, term_len);
					char* stripped = strip_outer_braces(term, term_len);
					int parsing_required = (term != stripped) ? PARSE_REQUIRED : PARSE_NOT_REQUIRED;
					tree_add(tree, stripped, strlen(stripped), prev_parm, parsing_required);

					prev_parm = (input[i] == operators[0] ? params[0] : params[1]);

					if (i+1 >= input_length) break;
					beg_pos = i+1;
					while (input[beg_pos] == ' ') ++beg_pos;
				}
				++i;
			}

			char* last_term = substring(input, beg_pos, input_length-beg_pos);
			char* stripped_last_term = strip_outer_braces(last_term, strlen(last_term));
			int parsing_required = (last_term != stripped_last_term) ? PARSE_REQUIRED : PARSE_NOT_REQUIRED;
			tree_add(tree, stripped_last_term, strlen(stripped_last_term), prev_parm, parsing_required);
		}

		else {
			char* input_copy = strndup(input, input_length);
			char* input_stripped = strip_outer_braces(input_copy, strlen(input_copy));
			int requires_parsing = (input_stripped != input_copy) ? PARSE_REQUIRED : PARSE_NOT_REQUIRED;
			tree_add(tree, input_stripped, input_length, PARAM_NOP, requires_parsing);
		}

	//tree_dump(tree);
	return tree;
	
	
}

// currently, there are ~8 levels of indentation in the following monstrosity :D

_double_t tree_get_result(tree_t *tree) {

	_double_t sum_elements[tree->num_terms];
	// assuming tree->priority_level == PRIO_ADD_SUB
	int i = 0;
	while (i < tree->num_terms) {
		if (tree->terms[i].requires_parsing) {
			tree_t *stree = tree_generate(tree->terms[i].string, tree->terms[i].length, PRIO_ADD_SUB);
			sum_elements[i] = tree_get_result(stree);
			tree_delete(stree);
		}
		else {
			tree_t *mtree = tree_generate(tree->terms[i].string, tree->terms[i].length, PRIO_MUL_DIV);
			_double_t mul_elements[mtree->num_terms];
			int j = 0;
			while (j < mtree->num_terms) {
				if (mtree->terms[j].requires_parsing) {
					tree_t *stree = tree_generate(mtree->terms[j].string, mtree->terms[j].length, PRIO_ADD_SUB);
					mul_elements[j] = tree_get_result(stree);
					tree_delete(stree);
				}
				else {
					tree_t *ttree = tree_generate(mtree->terms[j].string, mtree->terms[j].length, PRIO_POW_FAC);
					_double_t pow_elements[ttree->num_terms];
					int k = 0;
					while (k < ttree->num_terms) {
						if (ttree->terms[k].requires_parsing) {
							tree_t *stree = tree_generate(ttree->terms[k].string, ttree->terms[k].length, PRIO_ADD_SUB);
							pow_elements[k] = tree_get_result(stree);
							tree_delete(stree);
						}
						else { 
							int found;
							_double_t r = func_pass_get_result(ttree->terms[k].string, ttree->terms[k].length, &found);
							if (found) {
								pow_elements[k] = r;
							}
							else {
								pow_elements[k] = constant_pass_get_result(ttree->terms[k].string, ttree->terms[k].length);
							}
						}
						++k;
					}

					if (ttree->num_terms > 2) {
						_double_t pow_temp = pow_ptr(pow_elements[ttree->num_terms-2], pow_elements[ttree->num_terms-1]);
						k = ttree->num_terms-3;
						while (k >= 0) {
							// "right-associative"
							pow_temp = pow_ptr(pow_elements[k], pow_temp);							
							--k;
						}
						mul_elements[j] = pow_temp;
					} else if (ttree->num_terms == 2){
						mul_elements[j] = pow_ptr(pow_elements[0], pow_elements[1]);
					}
					else { mul_elements[j] = pow_elements[0]; }

					tree_delete(ttree);
				}
				++j;
			}
		
			_double_t mul_res = mul_elements[0];
			j = 1;
			while (j < mtree->num_terms) { 
				mul_res *= mtree->terms[j].param == PARAM_MUL ? mul_elements[j] : 1.0/mul_elements[j];
				++j;
			}

			sum_elements[i] = mul_res;
			tree_delete(mtree);
		}
		++i;
	}

	i = 1;
	_double_t sum_res = sum_elements[0];

	while(i < tree->num_terms) {
		// take actual sign into consideration
		sum_res += tree->terms[i].param == PARAM_SGN_POS ? sum_elements[i] : -sum_elements[i];
		++i;
	}
	return sum_res;
}

// prints a rough dump of the tree contents for debugging purposes
void tree_dump(tree_t *tree) {
	if (!tree) return;
	int i = 0;
	printf("(DEBUG): tree prio=%d:\n", tree->priority_level);
	for (;i < tree->num_terms; ++i) {
		printf("string: \"%s\" -> requires_parsing = %d, param = %d\n", tree->terms[i].string, tree->terms[i].requires_parsing, tree->terms[i].param);
	}
	printf("---------------\n");
}



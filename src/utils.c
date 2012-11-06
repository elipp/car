#include "utils.h"

extern const key_mathfuncptr_pair functions[];
extern const size_t functions_table_size;
extern const key_constant_pair constants[];
extern const size_t constants_table_size;
extern const key_funcptr_pair commands[];
extern const size_t commands_list_size;

#ifdef USE_CHEM_PLUGINS
extern const key_strfuncptr_pair chem_functions[];
extern const size_t chem_functions_table_size;
#endif

// use either strtold/strtod instead of atof.
inline _double_t to_double_t(const char* arg) { 
	char *end;
#ifdef C99_AVAILABLE 
	_double_t res = strtold(arg, &end);	// requires C99
#else
	_double_t res = strtod(arg, &end);
#endif
	return res;
}

inline int is_digit(char c) {
	return IS_DIGIT(c);
}

char* substring(const char* string, size_t pos, size_t length) {

	// consider: "if (length == 0) { return NULL; }"

	char* ret = malloc(length+1);
	memcpy(ret, &string[pos], length);
	ret[length] = '\0';
	return ret;

}

char *strip_outer_braces(char* term, size_t length) {

	char* ret;
	// check if expression is COMPLETELY surrounded by OUTER-LEVEL braces
	// if not, return original
	

	int initial_brace_pos = 0;
	while (term[initial_brace_pos] == ' ') ++initial_brace_pos;
	if (term[initial_brace_pos] == '(') {	// if first non-space character is '('
		int i = length-1;
		while (term[i] == ' ') --i;	
		if (term[i] == ')') {		// if last non-space character is ')'
			int k = initial_brace_pos+1;
			int num_brackets = 1;
			const int final_brace_pos = i;
			while (k < length) {
				if (term[k] == ')') {
					--num_brackets;
					if (num_brackets == 0) {
						if (k != final_brace_pos) {
							goto cant_strip;
						}
					}
				}
				else if (term[k] == '(') ++num_brackets;
				++k;
			}
			if (num_brackets == 0) {
				const size_t new_len = final_brace_pos - initial_brace_pos - 1;
				ret = substring(term, initial_brace_pos+1, new_len);
				free(term);
				return ret;
			}
			else {
				return NULL;	// error
			}
		}

	}
	else { goto cant_strip; }

cant_strip:
	return term;

}

char *strip_surrounding_whitespace(char* arg, size_t arg_len) {

	if (arg_len == 0) { return NULL; }
	int beg = 0;

	while (beg < arg_len) {
		if(arg[beg] != ' ') { break; }
		++beg;
	}
	if (beg == arg_len) { return arg; }
	int end = arg_len-1;
	while (end > beg) { 
		if (arg[end] != ' ') { break; }
		--end;
	}

	// if no change was detected
	if ((beg == 0) && (end == arg_len)) { return arg; }

	const size_t d = (end-beg)+1;
	char *ret = substring(arg, beg, d);

	free(arg);
	return ret;
}

char *strip_all_whitespace(char* arg, size_t arg_len) {
	size_t i = 0;
	// count whitespace
	size_t num_whitespace = 0;
	while (i < arg_len) {
		if (arg[i] == ' ') ++num_whitespace;
		++i;
	}
	if (num_whitespace == 0) {
		return arg;
	}
	// allocate memory
	const size_t stripped_len = arg_len - num_whitespace;// + 1; // we'll see about the +1
	char *stripped = malloc(stripped_len);

	i = 0;
	size_t j = 0;
	while (i < arg_len) {
		if (arg[i] != ' ') { stripped[j] = arg[i]; ++j; }
		++i;
	}
	stripped[stripped_len] = '\0';	// i just wonder, why is this working? should overflow, instead works as intended 
	free(arg);
	return stripped;
}

_double_t func_pass_get_result(const char* arg, size_t arg_len, int *found) {

	// find out if argument matches with any of the function names
	
	// preprocess arg for strcmp
	
	int k = 0;

	// clear possible characters from the beginning of the arg

	int num_braces = 0;
	while (k < arg_len) {
		if (arg[k] == '(') ++num_braces;
		else if (arg[k] == ')') --num_braces;
		else
		if (!IS_DIGIT(arg[k]) && (num_braces == 0)) { break; }
		++k;
	}
	if (num_braces != 0) { fprintf(stderr, "unmatched parenthesis @ \"%s\"\n", arg); *found = 0; return 0; }
	if (k == arg_len) {
		// some weird error has occurred :P
		*found = 0;
		return to_double_t(arg);
	}
	int factorial_found = 0;
	const int word_beg_pos = k;

	while (k < arg_len) {
		//if (!IS_LOWERCASE_LETTER(arg[k])) { // not enforcing only lowercase function identifiers
		if (!IS_LETTER(arg[k])) {
			while(arg[k] == ' ') ++k; 
			if (arg[k] == '!') factorial_found = 1; 
			break; 
		}
		++k;
	}

	if (k == arg_len) {
		// lolz, no right-hand argument?
		// this could present a wonderful opportunity to do a constant pass
	}
	const int word_end_pos = k;

	char *func_string = substring(arg, word_beg_pos, word_end_pos-word_beg_pos);

	int i = 0;
	while (i < functions_table_size) {
		if (strcmp(func_string, functions[i].key) == 0) { break; }
		++i;
	}
#ifdef USE_CHEM_PLUGINS
	if (strcmp(func_string, chem_functions[0].key) == 0) { 
		char* right_arg = substring(arg, word_end_pos, arg_len - word_end_pos);
		// no trees need to be generated
		_double_t r = chem_functions[0].funcptr(right_arg);
		free(right_arg);
		free(func_string);
		*found = 1;
		return r;
	}
#endif

	free(func_string);

	if ((i == functions_table_size) && (factorial_found == 0)) { *found = 0; return to_double_t(arg); }
	// else find function arg(s)
	
	else {

		double l = 1.0;
		const size_t left_length = word_beg_pos;

		if (left_length > 0) {
			char *left_arg = substring(arg, 0, left_length);
			tree_t *l_stree = tree_generate(left_arg, left_length, PRIO_ADD_SUB);
			l = tree_get_result(l_stree);
			free(left_arg);
			tree_delete(l_stree);

			// the factorial function doesn't have a right operand
			if (factorial_found) {
				*found = 1;
				return func_factorial(l);
			}
		}

		const size_t right_beg_pos = word_end_pos;
		char *right_arg = substring(arg, right_beg_pos, arg_len - right_beg_pos);
		tree_t *r_stree = tree_generate(right_arg, strlen(right_arg), PRIO_ADD_SUB);
		double r = 0.0;

		r = l*functions[i].funcptr(tree_get_result(r_stree));

		free(right_arg);
		tree_delete(r_stree);
		*found = 1;
		return r;
	}

}

_double_t constant_pass_get_result(const char* arg, size_t arg_len) {

	int k = 0;

	int num_braces = 0;
	while (k < arg_len) {
		if (arg[k] == '(') ++num_braces;
		else if (arg[k] == ')') --num_braces;
		else
		if (!IS_DIGIT(arg[k]) && (num_braces == 0)) { break; }
		++k;
	}
	// probably not even theoretically possible, but better to be sure
	if (num_braces != 0) { fprintf(stderr, "unmatched parenthesis @ %s\n", arg); return 0; }
	if (k == arg_len) {
		// only numbers were found, return to_double_t.
		return to_double_t(arg);
	}

	const int word_beg_pos = k;

	char *word = substring(arg, word_beg_pos, arg_len-word_beg_pos);
	// strip possible whytespace :P
	word = strip_surrounding_whitespace(word, strlen(word));
	int i = 0;

	// first, go through builtins
	
	const key_constant_pair *match = NULL;

	while(i < constants_table_size) {
		if (strcmp(word, constants[i].key) == 0) { match = &constants[i]; break; }
		++i;
	} if (i == constants_table_size) { 
		i = 0;
		const size_t udctree_sz = udctree_get_num_nodes();
		while (i < udctree_sz) {
			key_constant_pair *p = udctree_get(i);
			if (strcmp(word, p->key) == 0) { match = p; break; }
			++i;
		}
	}

	if (match == NULL) { free(word); return to_double_t(arg); }

	else {
		_double_t r = 1.0;
		if (word_beg_pos > 0) {
			char *left = substring(arg, 0, word_beg_pos);
			tree_t *stree = tree_generate(left, strlen(left), PRIO_ADD_SUB);
			r = tree_get_result(stree);
			tree_delete(stree);
			free(left);
		}
		free(word);
		return r*match->value;
	}

}

_double_t parse_mathematical_input(char* arg) {
	tree_t *stree = tree_generate(arg, strlen(arg), PRIO_ADD_SUB);
	if (stree) {
		_double_t res = tree_get_result(stree);
		tree_delete(stree);
		return res;
	}
	else { return 0; }

}

// decompose argument into a list of words (delimited by whitespace, strtok)
word_list *wlist_generate(const char* arg) {

	word_list *ret = malloc(sizeof(word_list));
	ret->num_words = 0;
	char *arg_copy = strdup(arg);

	static const char* delims = " ";
	
	char *token = strtok(arg_copy, delims);	
	while (token != NULL) {
		wlist_add(ret, strdup(token));
		token = strtok(NULL, delims);
	}	
	
	free(arg_copy);
	return ret;
}

void wlist_add(word_list *list, char *arg) {

	word_node *newnode = malloc(sizeof(word_node));
	// should a copy be taken? :P
	newnode->text = arg;
	newnode->length = strlen(arg);

	if (list->num_words == 0) {
		list->head = newnode;
		list->root = newnode;
		newnode->next = NULL;
	}
	else {
		list->head->next = newnode;
		list->head = newnode;
		newnode->next = NULL;
	}
	++list->num_words;
}

char *wlist_get(word_list *list, int index) {
	const size_t num_words = list->num_words;
	if (index < num_words) {
		int i = 0;
		word_node *iter = list->root;
		while (i < index) {
			iter = iter->next;
			++i;
		}
		return iter->text;
	
	} else return NULL;
}

void wlist_print(word_list *list) {
	int i = 0;

	word_node *node = list->root;
	while (i < list->num_words) {
		printf("#%d: %s, length %lu\n", i, node->text, node->length);
		node = node->next;
		++i;
	}

}

char *wlist_recompose(word_list *list, size_t *length) {
	// find out total combined size
	size_t sz = 0;

	word_node *iter;
	for (iter = list->root; iter != NULL; iter = iter->next) {
		sz += iter->length + 1;	// need to reserve space for the whitespace (' ') character
	}
	--sz;	// the last element doesn't need a trailing ' '
	*length = sz;
	char *ret = malloc(sz);
	ret[0] = '\0';

	for (iter = list->root; iter->next != NULL; iter = iter->next) {
		strcat(ret, iter->text);
		strcat(ret, " ");
	}
	strcat(ret, iter->text);

	return ret;
}

void wlist_delete(word_list *list) {

	word_node *iter = list->root;
	word_node *nexttmp = iter->next;
	while (nexttmp != NULL) {
		free(iter->text);
		free(iter);
		iter = nexttmp;
		nexttmp = nexttmp->next;
	}
	free(iter->text);
	free(iter);
	free(list);

}

int wlist_parse_command(word_list *list) {
	
	// extern key_funcptr_pair commands[];
	// extern const size_t commands_list_size;
	
	int i = 0;
	const char *first = list->root->text;

	while (i < commands_list_size) {
		if (strcmp(first, commands[i].key) == 0) { break; }
		++i;
	}
	if (i == commands_list_size) {
		return 0;
	}
	else {
		commands[i].func(list);
		return 1;
	}

}


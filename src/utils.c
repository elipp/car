#include "utils.h"

extern const key_funcptr_pair functions[];
extern const size_t functions_table_size;
extern const key_constant_pair constants[];
extern const size_t constants_table_size;

static const unsigned char lcase_letters_lower_bound = 'a';
static const unsigned char lcase_letters_upper_bound = 'z';
static const unsigned char ucase_letters_lower_bound = 'A';
static const unsigned char ucase_letters_upper_bound = 'Z';
static const unsigned char numbers_lower_bound = '0';
static const unsigned char numbers_upper_bound = '9';

// use either strtold/strtod instead of atof.
static inline _double_t to_double_t(const char* arg) { 
	char *end;
#ifdef LONG_DOUBLE_PRECISION
	_double_t res = strtold(arg, &end);
#else
	_double_t res = strtod(arg, &end);
#endif
	return res;
}


char* substring(const char* string, size_t pos, size_t length) {

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
	int beg = 0;

	while (beg < arg_len) {
		if(arg[beg] != ' ') { break; }
		++beg;
	}
	if (beg == arg_len) { return NULL; }
	int end = arg_len-1;
	while (end > beg) { 
		if (arg[end] != ' ') { break; }
		--end;
	}

	if (beg == 0 && end == arg_len-1) { return arg; }

	const size_t d = beg-end;
	char *ret = substring(arg, beg, arg_len - d);

	free(arg);
	return ret;
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
		if (!IS_NUMBER(arg[k]) && (num_braces == 0)) { break; }
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
		if (!IS_LOWERCASE_LETTER(arg[k])) { 
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
//			printf("func_pass: left_arg = %s, l = %f\n", left_arg, l);
			free(left_arg);
			tree_delete(l_stree);

			if (factorial_found) {
				*found = 1;
				return func_factorial(l);
			}
		}

		const size_t right_beg_pos = word_end_pos;
		char *right_arg = substring(arg, right_beg_pos, arg_len - right_beg_pos);
		tree_t *r_stree = tree_generate(right_arg, strlen(right_arg), PRIO_ADD_SUB);
		double r = 0.0;

		// check for factorial :P
		r = l*functions[i].funcptr(tree_get_result(r_stree));

//		printf("func_pass: right_arg = %s, r = %f\n", right_arg, r);
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
		if (!IS_NUMBER(arg[k]) && (num_braces == 0)) { break; }
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
	int i = 0;

	while(i < constants_table_size) {
		if (strcmp(word, constants[i].key) == 0) { break; }
		++i;
	}

	if (i == constants_table_size) { free(word); return to_double_t(arg); }
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
		return r*constants[i].value;
	}

}

_double_t parse_input(char* arg) {
	tree_t *stree = tree_generate(arg, strlen(arg), PRIO_ADD_SUB);
	if (stree) {
		_double_t res = tree_get_result(stree);
		tree_delete(stree);
		return res;
	}
	else { return 0; }

}

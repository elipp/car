#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <string.h>
#include "definitions.h"
#include "functions.h"
#include "tree.h"
#include "ud_constants_tree.h"

#define IS_LOWERCASE_LETTER(x) (((x) >= lcase_letters_lower_bound && (x) <= lcase_letters_upper_bound))
#define IS_UPPERCASE_LETTER(x) (((x) >= ucase_letters_lower_bound && (x) <= ucase_letters_upper_bound)) 
#define IS_LETTER(x) 	       (IS_LOWERCASE_LETTER(x) || IS_UPPERCASE_LETTER(x))

#define IS_DIGIT(x) ((((x) >= numbers_lower_bound && (x) <= numbers_upper_bound)) || (x) == '.')
#define IS_DIGIT_OR_BRACE(x) ((IS_NUMBER(x) || (x == '(' || x == ')')))

inline _double_t to_double_t(const char*);
inline int is_digit(char c);

_double_t func_pass_get_result(const char* arg, size_t arg_len, int *found);
_double_t constant_pass_get_result(const char* arg, size_t arg_len);
char *substring(const char* string, size_t pos, size_t length);
char *strip_outer_braces(char* term, size_t length);
char *strip_surrounding_whitespace(char* arg, size_t length);

// wrapper for tree_get_result
_double_t parse_mathematical_input(char* arg);

word_list *wlist_generate(const char* arg);
void wlist_add(word_list *list, char* arg);
char *wlist_get(word_list *list, int index);
char *wlist_recompose(word_list *list, size_t *lengt);
void wlist_delete(word_list *list);

// debug print
void wlist_print(word_list *list);
	
#endif 


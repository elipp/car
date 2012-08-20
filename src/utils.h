#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <string.h>
#include "definitions.h"
#include "functions.h"
#include "tree.h"

#define IS_LOWERCASE_LETTER(x) (((x) >= lcase_letters_lower_bound && (x) <= lcase_letters_upper_bound))
#define IS_UPPERCASE_LETTER(x) (((x) >= ucase_letters_lower_bound && (x) <= ucase_letters_upper_bound)) 
#define IS_LETTER(x) 	       (IS_LOWERCASE_LETTER(x) || IS_UPPERCASE_LETTER(x))

#define IS_NUMBER(x) ((((x) >= numbers_lower_bound && (x) <= numbers_upper_bound)) || (x) == '.')
#define IS_NUMBER_OR_BRACE(x) ((IS_NUMBER(x) || (x == '(' || x == ')')))

_double_t func_pass_get_result(const char* arg, size_t arg_len, int *found);
_double_t constant_pass_get_result(const char* arg, size_t arg_len);
char* substring(const char* string, size_t pos, size_t length);
char *strip_outer_braces(char* term, size_t length);

#endif 


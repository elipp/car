#include "chem.h"
#include "../utils.h"	// for the macros

static _double_t get_atomic_mass(const char*, int*);

static _double_t get_atomic_mass(const char* arg, int *found) {

	int i = 0;
	while (i < atomic_weight_table_size) {
		if (strcmp(arg, atomic_weight_table[i].key) == 0) break;
		++i;
	}

	if (i >= atomic_weight_table_size) {
		printf("Chemical element \"%s\" not found in the table! Returning -1 for molar mass.\n", arg);
		*found = 0;
		return 0.0;
	}
	
	else {
		*found = 1;
		return atomic_weight_table[i].value;
	}

}

_double_t func_molar_mass(const char* arg) {

	// the func_pass_get_result passes the inner argument WITH the outer braces included.

	const size_t arg_len = strlen(arg);
	char *stripped = strip_outer_braces(strndup(arg, arg_len), arg_len);
	const size_t stripped_len = strlen(stripped);
	
	size_t i = 0;
	_double_t sum = 0.0;

	while (i < stripped_len) {
		if (IS_UPPERCASE_LETTER(stripped[i])) {
			size_t elem_beg_pos = i;
			// go forth until another uppercase letter is encountered
			if (i >= stripped_len) break;	// needs to be handled differently tho

			do {	// one of the few instances where do...while is actually useful
				++i;
			} while (IS_LOWERCASE_LETTER(stripped[i]) && i < stripped_len);

			size_t elem_end_pos = i;

			// find out if there's a digit char trailing the element

			_double_t factor = 1.0;
			size_t factor_beg_pos = i;
			while (IS_DIGIT(stripped[i]) && i < stripped_len) {
				++i;
			}
			size_t factor_end_pos = i;
			if (i != factor_beg_pos) {
				// trailing digits were found, strtol(d)
				char *factor_string = substring(stripped, factor_beg_pos, factor_end_pos - factor_beg_pos);
				char *dummy;	// required by strtold 
				factor = strtold(factor_string, &dummy);
				free(factor_string);
			} 

			char *elem = substring(stripped, elem_beg_pos, elem_end_pos-elem_beg_pos);
			int found;
			_double_t elem_molar_mass = get_atomic_mass(elem, &found);

			if (found == 0) {
				sum=0;
				break;
			}
			else {
				sum += factor * elem_molar_mass;
			}

			free(elem);
		}

	}

	free(stripped);

	return sum;
}


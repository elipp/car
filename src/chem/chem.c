#include "chem.h"
#include "atomic_weights.h"

const size_t atomic_mass_table_size = sizeof(atomic_mass_table)/sizeof(atomic_mass_table[0]);

static _double_t get_atomic_mass(const char* arg) {

	const key_constant_pair *iter = &atomic_mass_table[0];
	
	int i = 0;
	while (i < atomic_mass_table_size) {
		if (strcmp(arg, atomic_mass_table[i].key) == 0) break;
		++i;
	}

	if (i >= atomic_mass_table_size) {
		printf("Chemical element \"%s\" not found in the table! Returning -1 for molar mass.\n", arg);
		return -1.0;
	}
	
	else return atomic_mass_table[i].value;

}

_double_t func_molar_mass(const char* arg) {
	// assuming the whole M(...) has been passed in :P
	const size_t braced_len = strlen(arg) - 1;
	char *t = substring(arg, 1, braced_len);
	char *inner = strip_outer_braces(t, braced_len);

	const size_t inner_len = strlen(inner);

	// ok, start parsing
	
	int i = 0;
	size_t beg_pos = 0;
	size_t end_pos = 0;;
	
	while (i < inner_len) {
		if(IS_UPPERCASE_LETTER(arg[i])) {
			// an uppercase letter always marks the beginning of a new term
			char *xd = substring(arg, beg_pos, end_pos - beg_pos);
			printf("%s, ", xd);
			// then do a lookup from the atomic mass table and stuff
			free(xd);
			beg_pos = i;
			end_pos = i;

		}
		else if (IS_LOWERCASE_LETTER(arg[i])) {
			end_pos = i;
		}

		else if (IS_DIGIT(arg[i])) {

		}	
		++i;
	}
	putchar('\n');
	return (_double_t) 0.0;

}

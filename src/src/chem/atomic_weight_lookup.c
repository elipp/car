#include "atomic_weights.h"
#include "../utils.h"

const size_t atomic_mass_table_size = sizeof(atomic_mass_table)/sizeof(atomic_mass_table[0]);

_double_t get_atomic_mass(const char* arg) {

	const key_constant_pair *iter = &atomic_mass_table[0];
	
	int i = 0;
	while (i < atomic_mass_table_size) {
		if (strcmp(arg, atomic_mass_table[i]) == 0) break;
		++i;
	}

	if (i >= atomic_mass_table_size) {
		printf("Chemical element \"%s\" not found in the table! Returning -1 for molar mass.\n", arg);
		return -1.0;
	}
	
	else return atomic_mass_table[i].value;

}

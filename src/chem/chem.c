#include "chem.h"
#include "atomic_weights.h"

static const size_t atomic_mass_table_size = 
sizeof(atomic_mass_table)/sizeof(atomic_mass_table[0]);

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

}


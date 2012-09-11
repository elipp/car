#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <math.h>
#include <stdio.h>
#include "definitions.h"

#ifdef USE_CHEM_PLUGINS
#include "chem/atomic_weights.h"
_double_t func_molar_mass(const char* arg);
#endif

/* binary operators */

// the binary operators are stubs :P
_double_t func_binary_and(_double_t a, _double_t nil);
_double_t func_binary_or(_double_t a, _double_t nil);
_double_t func_factorial(_double_t a);


#endif


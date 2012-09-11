#include "functions.h"

#ifdef LONG_DOUBLE_PRECISION
_double_t (*floor_ptr)(_double_t) = floorl;
_double_t (*tgamma_ptr)(_double_t) = tgammal;
#else
_double_t (*floor_ptr)(_double_t) = floor; 
_double_t (*tgamma_ptr)(_double_t) = tgamma;
#endif

_double_t func_factorial(_double_t a) {
	if (floor_ptr(a) != a) {
		printf("func_factorial: error: non-integer operand!\n");
		return 0.0;
	}
	else if (a == 0) { return 1.0; }

	_double_t res = 1;
	int i = a;
	while (i > 0) {
		res *= i;	
		--i;
	}
	return res;
}

_double_t func_binary_and(_double_t a, _double_t b) { return (int)a&(int)b; }
_double_t func_binary_or(_double_t a, _double_t b) { return (int)a|(int)b; }

#ifdef USE_CHEM_PLUGINS
_double_t func_molar_mass(const char* arg) {
	// assuming the whole M(...) has been passed in :P
	const size_t braced_len = strlen(arg) - 1;
	char *t = substring(arg, 1, braced_len);
	char *inner = strip_outer_braces(t, braced_len);

	const size_t inner_len = strlen(inner);

	// ok, start parsing
	
	int i = 0;
	size_t beg_pos = 0;
	size_t end_pos;
	
	while (i < inner_len) {
		if(IS_UPPERCASE_LETTER(arg[i])) {
			// an uppercase letter always marks the beginning of a new term
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

}
#endif

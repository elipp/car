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


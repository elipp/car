#include "functions.h"

#ifdef LONG_DOUBLE_PRECISION
	_double_t (*floor_ptr)(_double_t) = floorl;
	#ifdef C99_AVAILABLE
	_double_t (*tgamma_ptr)(_double_t) = tgammal;
	#endif
#else
	_double_t (*floor_ptr)(_double_t) = floor; 
	#ifdef C99_AVAILABLE
	_double_t (*tgamma_ptr)(_double_t) = tgamma;
	#endif
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
_double_t func_deg(_double_t rad) { return (360.0L*rad)/(2*M_PI); }
_double_t func_rad(_double_t deg) { return (2*M_PI*deg)/360.0L; }


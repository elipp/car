#include "definitions.h"

#include "functions.h"

#ifdef LONG_DOUBLE_PRECISION
const key_funcptr_pair functions[] = 	   { { "cos", cosl }, { "sin", sinl }, { "tan", tanl },
					     { "acos", acosl }, { "asin", asinl }, { "atan", atanl },
					     { "exp", expl }, { "ln", logl }, { "log", log10l },
					     { "sqrt", sqrtl }, { "abs", fabsl }, { "gamma", tgammal },
					     { "cosh", coshl }, { "sinh", sinhl }, { "tanh", tanhl },
					     { "acosh", acoshl }, { "asinh", asinhl }, { "atanh", atanhl } };

const key_constant_pair constants[] = { { "pi",  3.14159265358979323846264338327950288L },
					{ "Pi",  3.14159265358979323846264338327950288L },
					{ "PI",  3.14159265358979323846264338327950288L },
					{ "e",   2.71828182845904523536028747135266249L } };

#else

const key_funcptr_pair functions[] = 	   { { "cos", cos }, { "sin", sin }, { "tan", tan },
					     { "acos", acos }, { "asin", asin }, { "atan", atan },
					     { "exp", exp }, { "ln", log }, { "log", log10 },
					     { "sqrt", sqrt }, { "abs", fabs }, { "gamma", tgammal },
					     { "cosh", cosh }, { "sinh", sinh }, { "tanh", tanh },
					     { "acosh", acosh }, { "asinh", asinh }, { "atanh", atanh } };

const key_constant_pair constants[] = { { "pi",  3.14159265358979323846 },
					{ "Pi",  3.14159265358979323846 },
					{ "PI",  3.14159265358979323846 },
					{ "e",   2.71828182845904523536 } };


#endif

const size_t functions_table_size = sizeof(functions)/sizeof(functions[0]);
const size_t constants_table_size = sizeof(constants)/sizeof(constants[0]);


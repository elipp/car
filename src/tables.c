#include "definitions.h"
#include "functions.h"

#ifdef LONG_DOUBLE_PRECISION
const key_mathfuncptr_pair functions[] =     { { "cos", cosl }, { "sin", sinl }, { "tan", tanl },
					     { "acos", acosl }, { "asin", asinl }, { "atan", atanl },
					     { "exp", expl }, { "ln", logl }, { "log", log10l },
					     { "sqrt", sqrtl }, { "abs", fabsl }, { "cosh", coshl },
					     { "sinh", sinhl }, { "tanh", tanhl },{ "acosh", acoshl }, 
					     { "asinh", asinhl }, { "atanh", atanhl }, 
	#ifdef C99_AVAILABLE 
					     { "gamma", tgammal },
	#endif
					     { "deg", func_deg }, { "rad", func_rad }
};

const key_constant_pair constants[] = { { "pi",  3.14159265358979323846264338327950288L },
					{ "Pi",  3.14159265358979323846264338327950288L },
					{ "PI",  3.14159265358979323846264338327950288L },
					{ "e",   2.71828182845904523536028747135266249L },
					{ "c",	 299792458L },
					{ "h", 	 6.62606957e-34L },
					{ "m_e", 9.10938291e-31L },
					{ "m_p", 1.672621777e-27L },
					{ "m_n", 1.674927351e-27L },
					{ "m_u", 1.660538921e-27L },
					{ "N_A", 6.02214129e23L },
					{ "R", 8.3144621L }

};

#else

const key_mathfuncptr_pair functions[] =    { { "cos", cos }, { "sin", sin }, { "tan", tan },
					     { "acos", acos }, { "asin", asin }, { "atan", atan },
					     { "exp", exp }, { "ln", log }, { "log", log10 },
					     { "sqrt", sqrt }, { "abs", fabs },{ "cosh", cosh }, 
					     { "sinh", sinh }, { "tanh", tanh }, { "acosh", acosh }, 
					     { "asinh", asinh }, { "atanh", atanh }, 
	#ifdef C99_AVAILABLE 
					     { "gamma", tgamma },
	#endif
					     { "deg", func_deg }, { "rad", func_rad }
};

const key_constant_pair constants[] = { { "pi",  3.14159265358979323846264338327950288L },
					{ "Pi",  3.14159265358979323846264338327950288L },
					{ "PI",  3.14159265358979323846264338327950288L },
					{ "e",   2.71828182845904523536028747135266249L },
					{ "c",	 299792458L },
					{ "h", 	 6.62606957e-34L },
					{ "m_e", 9.10938291e-31L },
					{ "m_p", 1.672621777e-27L },
					{ "m_n", 1.674927351e-27L },
					{ "m_u", 1.660538921e-27L },
					{ "N_A", 6.02214129e23L },
					{ "R", 8.3144621L }

};




#endif

const size_t functions_table_size = sizeof(functions)/sizeof(functions[0]);
const size_t constants_table_size = sizeof(constants)/sizeof(constants[0]);

#ifdef USE_CHEM_PLUGINS
const key_strfuncptr_pair chem_functions[] = { "molarmass", func_molar_mass };
const size_t chem_functions_table_size = sizeof(chem_functions)/sizeof(chem_functions[0]);
#endif


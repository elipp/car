#include "definitions.h"
#include "functions.h"

#ifdef LONG_DOUBLE_PRECISION
const key_mathfuncptr_pair functions[] =   { { "cos", cosl }, { "sin", sinl }, { "tan", tanl },
					     { "acos", acosl }, { "asin", asinl }, { "atan", atanl },
					     { "exp", expl }, { "ln", logl }, { "log", log10l },
					     { "sqrt", sqrtl }, { "abs", fabsl }, { "cosh", coshl },
					     { "sinh", sinhl }, { "tanh", tanhl },{ "acosh", acoshl }, 
					     { "asinh", asinhl }, { "atanh", atanhl }, { "gamma", tgammal }
#ifdef USE_CHEM_PLUGINS
					   , { "molar_mass", func_molar_mass }
#endif
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
					     { "asinh", asinh }, { "atanh", atanh }, { "gamma", tgamma }
#ifdef USE_CHEM_PLUGINS
					   , { "molar_mass", func_molar_mass }
#endif
};

const key_constant_pair constants[] = { { "pi",  3.14159265358979323846 },
					{ "Pi",  3.14159265358979323846 },
					{ "PI",  3.14159265358979323846 },
					{ "e",   2.71828182845904523536 } 
					{ "c",	 299792458 },
					{ "h", 	 6.62606957e-34 }
					{ "m_e", 9.10938291e-31 },
					{ "m_p", 1.672621777e-27 },
					{ "m_n", 1.674927351e-27 },
					{ "m_u", 1.660538921e-27 },
					{ "N_A", 6.02214129e23 },
					{ "R", 8.3144621 }
};




#endif

const size_t functions_table_size = sizeof(functions)/sizeof(functions[0]);
const size_t constants_table_size = sizeof(constants)/sizeof(constants[0]);



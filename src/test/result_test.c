#include "../definitions.h"
#include "../tree.h"
#include "../utils.h"

typedef struct {
	char* expr;
	_double_t result;
} test_t;

// correct results gathered from wolframalpha
static const test_t tests[] = {
{ "(5+1*2)-23^2+1", -521 },
{ "sin(e*sin(sinh(pi/2)))", 0.8987498494059207598786 },
{ "(54/6)! + 1/2", 362880.5 },
{ "(2.1)^(-1.2)", 0.410522387808581614598 },
{ "(e)^(2pi+2e)", 122976.5497941034940 },
{ "e^(2pi+2e)", 122976.5497941034940 },
{ "sin(sin(sin(-1.0)))", -0.6784304773607 },
{ "2*((25+((532+1)/2^2)+2.1*2*3/(1.0-2.0)*sin(1.53)))/3.0 + 1", 98.106989259719 },
{ "-(1.35*3sin(2))/(2+1)", -1.227551526214670 },
{ "-sin(5 + sin( cos(2.0)/2.0))", 0.996718399031256 }
};


#ifdef LONG_DOUBLE_PRECISION
#define RES_STRING "got\t\t%.14Lg\nshould be\t%.14Lg\n\n", real_res, tests[i].result
#else
#define RES_STRING "got\t\t%.8g\nshould be \t%.8g\n\n", real_res, tests[i].result
#endif

static const _double_t threshold = 0.00001;

int main(int argc, char* argv[]) {
	static const size_t tests_size = sizeof(tests)/sizeof(tests[0]);
	int i = 0;
	int passed = 0;

	printf("calc: Running a (very) small test suite, testing the parser for correctness, not accuracy.\n\n", threshold);
	while (i < tests_size) {

		printf("expr: \"%s\"\n", tests[i].expr);
<<<<<<< HEAD
		_double_t real_res = parse_mathematical_input(tests[i].expr);
		printf(RES_STRING);
=======
		char *current_expr = strndup(tests[i].expr, strlen(tests[i].expr));
		//current_expr = strip_all_whitespace(current_expr, strlen(current_expr));
		_double_t real_res = parse_mathematical_input(current_expr);
		printf("got\t\t%.14Lg\nshould be\t%.14Lg\n\n", real_res, tests[i].result);
>>>>>>> fixed

		_double_t delta = tests[i].result - real_res;
		if (fabsl(delta) > threshold) {
			printf("(result delta exceeded threshold value %f!)\n", threshold);
		} else { ++passed; }
		++i;
		free(current_expr);
	}

	printf("RESULTS:\npassed: %d\ntotal:  %lu.\n", passed, tests_size); 
	return 0; 
}

#include "definitions.h"
#include "tree.h"
#include "utils.h"

typedef struct {
	char* expr;
	_double_t result;
} test_t;

// results gathered from wolframalpha
static const test_t tests[] = {
{ "(5+1*2)-23^2+1", -521 },
{ "sin(e*sin(sinh(pi/2)))", 0.898749849 },
{ "(54/6)! + 1/2", 362880.5 },
{ "(2.1)^(-1.2)", 0.410522388 },
{ "(e)^(2pi+2e)", 122976.5497941034940 },
{ "e^(2pi+2e)", 122976.5497941034940 },
{ "sin(sin(sin(-1.0)))", -0.6784304773607 },
{ "2*((25+((532+1)/2^2)+2.1*2*3/(1.0-2.0)*sin(1.53)))/3.0 + 1", 98.106989259719 },
{ "-(1.35*3sin(2))/(2+1)", -1.227551526214670 }
};


static const _double_t threshold = 0.00001;

int main(int argc, char* argv[]) {
	static const size_t tests_size = sizeof(tests)/sizeof(tests[0]);
	int i = 0;
	int passed = 0;

	printf("Running a (very) small test suite, testing the parser for correctness, not accuracy.\n\n", threshold);
	while (i < tests_size) {

		printf("expr: \"%s\"\n", tests[i].expr);
		_double_t real_res = parse_input(tests[i].expr);
		printf("got\t\t%.10Lf\nshould be\t%.10Lf\n\n", real_res, tests[i].result);

		_double_t delta = tests[i].result - real_res;
		if (fabsl(delta) > threshold) {
			printf("(result delta exceeded threshold value %Lf!)\n", threshold);
		} else { ++passed; }
		++i;
	}

	printf("RESULTS:\npassed: %d\ntotal:  %lu.\n", passed, tests_size); 
	return 0; 
}

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>

#include "definitions.h"
#include "tree.h"
#include "utils.h"

#ifdef LONG_DOUBLE_PRECISION
static const char* intfmt = "= \033[1;29m%1.0Lf\033[m\n";
static const char* fracfmt = "= \033[1;29m%4.18Lf\033[m\n";
static const char* welcome = "calc. using long double.";
#else
static const char* intfmt = "= \033[1;29m%1.0f\033[m\n";
static const char* fracfmt = "= \033[1;29m%4.14f\033[m\n";
static const char* welcome = "calc. using double-precision floating point";
#endif
static const size_t typesize = sizeof(_double_t) * 8;

extern _double_t (*floor_ptr)(_double_t);

int main(int argc, char* argv[]) {

//	struct timeval tv_start;
//	struct timeval tv_end;

	char *input;
	printf("%s (sizeof(_double_t): %lu bits)\n", welcome, typesize);

	for (;;) {

		input = readline("");
		const size_t input_length = strlen(input);
		char *input_stripped = strip_surrounding_whitespace(input, input_length);
		if (input_stripped) { 

		// check if any of the built-in command keywords have been input :P
			_double_t result = parse_input(input_stripped);
			if (floor_ptr(result) == result) {
				printf(intfmt, result);
			} else { printf(fracfmt, result); }

						
		}
		add_history(input_stripped);
		free(input_stripped);
	}


	return 0;

}

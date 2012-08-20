#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>

#include "definitions.h"
#include "tree.h"

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

		// check if any of the built-in command keywords have been input :P

//		gettimeofday(&tv_start, NULL);

		// create a level-0 priority tree
		tree_t *stree = tree_generate(input, strlen(input), PRIO_ADD_SUB);
		if (stree) {  
			_double_t res = tree_get_result(stree);
			if (floor_ptr(res) == res) {
				printf(intfmt, res);	
			} else {
				printf(fracfmt, res);
			}				
						
		}
		add_history(input);
		free(input);

	}


	return 0;

}

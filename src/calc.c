#include <stdio.h>
#include <stdlib.h>

#define C99_AVAILABLE

#ifdef NO_GNU_READLINE
#include "vt100_readline_emul.h"
#else
#include <readline/readline.h>
#endif

#include "definitions.h"
#include "tree.h"
#include "utils.h"

#ifdef LONG_DOUBLE_PRECISION
static const char* intfmt = "= \033[1;29m%1.0Lf\033[m\n";
//static const char* fracfmt = "= \033[1;29m%1.16Lf\033[m\n";
static const char* fracfmt = "= \033[1;29m%.16Lg\033[m\n";
static const char* welcome = "calc. using long double precision";
#else
static const char* intfmt = "= \033[1;29m%1.0f\033[m\n";
static const char* fracfmt = "= \033[1;29m%.14g\033[m\n";
static const char* welcome = "calc. using double-precision floating point";
#endif
static const size_t typesize = sizeof(_double_t) * 8;

extern _double_t (*floor_ptr)(_double_t);

extern int quit_signal;

int main(int argc, char* argv[]) {

//	struct timeval tv_start;
//	struct timeval tv_end;

#ifdef NO_GNU_READLINE
	e_readline_init();
#endif
	char *input;
	#ifdef NO_GNU_READLINE
	printf("%s & vt100_readline_emul.\n", welcome);
	#else
	printf("%s & the GNU readline library.\n", welcome);
	#endif
	
	
	udc_node *ans = udc_node_create("ans", 0);
	udctree_add(ans);

	while (quit_signal == 0) {
		#ifdef NO_GNU_READLINE
		input = e_readline("");
		#else
		input = readline("");
		#endif
		if (!input) continue;	// to counter ^A^D (ctrl+A ctrl+D) segfault
		const size_t input_length = strlen(input);
		char *input_stripped = strip_surrounding_whitespace(strdup(input), input_length);

		if (input_stripped) { 
			word_list *wlist = wlist_generate(input_stripped);
			int found = wlist_parse_command(wlist);
			if (found == 0) {
				// no matching command was found, parse as mathematical input -> all whitespace can now be filtered, to simplify parsing
				input_stripped = strip_all_whitespace(input, input_length);
				if (!input_stripped) { goto cont; }	// strip_all_whitespace returns NULL if input is completely whitespace
				_double_t result = parse_mathematical_input(input_stripped);

				if (floor_ptr(result) == result) {
					printf(intfmt, result);
				} else { printf(fracfmt, result); }
				
				#ifdef NO_GNU_READLINE
				e_hist_add(input_stripped);
				#else 
				add_history(input_stripped);
				#endif 
				ans->pair.value = result;

			}
		cont:
			wlist_delete(wlist);
						
		}
		free(input_stripped);
	}

	#ifdef NO_GNU_READLINE
	e_readline_deinit();
	#endif

	return 0;

}

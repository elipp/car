#include "commands.h"
#include "functions.h"

#define PRECSTRING_MAX 128

char f_precision[PRECSTRING_MAX] = { 0 };	// this isn't exactly the place for this, but...

extern const key_mathfuncptr_pair functions[];
extern const size_t functions_table_size;
extern const key_constant_pair constants[];
extern const size_t constants_table_size;

extern int clashes_with_predefined(const char*);	// from tables.c

int quit_signal = 0;

CMD_MAIN my(word_list *wlist) {

	// add variable to user-defined constant list (udc_list)
	// check that the my-invocation was properly formatted
	
	if (wlist->num_words > 1) {
		// blocks like these could be replaced with some function pointer action
		if (strcmp(wlist_get(wlist, 1), "list") == 0) {
			my_list();			
		}
		else {
			size_t recomposed_length;
			char *recomposed = wlist_recompose(wlist, &recomposed_length);

			// could be retokenized with respect to '='; greatly facilitates parsing
			
			word_list *retokenized = wlist_generate(recomposed, "=");
			
			if (retokenized->num_words != 2) { 
				printf("my: error: exactly 1 \'=\' expected. See \"help my\".\n"); 
				wlist_delete(&retokenized);
				free(recomposed);
				return;
			}
			
			char* varname = wlist_get(wlist, 1);	// the second word should be the desired variable name
			char* valstring = wlist_get(retokenized, 1);	// token after the '=' char

			if (IS_DIGIT(varname[0])) {
				printf("\nmy: error: variable identifiers cannot begin with digits.\n");
			}
			else if (clashes_with_predefined(varname)) {
					// the call prints its own error message
			}
			else {
				// check if already exists in the udctree.
				udc_node *search_result = udctree_search(varname);
				if (!search_result) {
					udc_node *newnode = malloc(sizeof(udc_node)); 
					newnode->pair.key = strdup(varname);
					newnode->pair.value = parse_mathematical_input(valstring);
					udctree_add(newnode);
				}
				else {
					search_result->pair.value = parse_mathematical_input(valstring);
				}
			}

			free(recomposed);
			wlist_delete(&retokenized);
		}
	}

	else {
		printf("\nmy: missing operand - see \"help my\".\n");
	}

}


CMD_SUB my_list() {
	int i = 0;
	udc_node *iter = udctree_get_root();
	if (iter) {
		printf("\nUser-defined constants:\n\nkey\tvalue\n");
		while (iter) {
			printf(" %s\t %.8g\n", iter->pair.key, (double)iter->pair.value); 
			iter = iter->next;
			++i;
		}
		printf("\nTotal: %d\n", i);
	}
	else {
		printf("\nNo user-defined constants found. See \"help my\" for additional information.\n\n");
	}
}

CMD_MAIN help(word_list *wlist) {

	if (wlist->num_words == 1) {
	printf(
"\ncalc is a calculator application, meaning it parses input\n\
mathematical expressions and computes the corresponding answer.\n\
\n\
GNU readline(-style) line-editing and input history is supported.\n\
\n\
Try \"help functions\" for a list of supported functions,\n\
    \"help constants\" for a list of built-in constants,\n\
    \"help my\" for information on the \"my\" command.\n\
    \"help set\" for information on the \"set\" command.\n\
\n");
	} else {
		if (wlist->num_words == 2) {
			const char *keyword = wlist_get(wlist, 1);
			// the list of help indices is small enough to just do if strcmp...else if
			if (strcmp(keyword, "functions") == 0) {
				help_functions();
			}
			else if (strcmp(keyword, "constants") == 0) {
				help_constants();
			}
			else if (strcmp(keyword, "my") == 0) {
				help_my();
			}
			else if (strcmp(keyword, "set") == 0) {
				help_set();
			}
			else { 
				printf("\nhelp: unknown help index \"%s\".\n", keyword);
			}
		}
		else { printf("help: trailing character(s) (\"%s...\")\n", wlist_get(wlist,2)); }
	}

}

CMD_SUB help_functions() {

	static const size_t num_col = 6;
	// extern size_t functions_table_size
	printf("\nSupported mathematical functions:\n");
	size_t i = 0;
	size_t j = 0;
	while (j < functions_table_size/num_col) {
		i = 0;
		while (i < num_col) {
			printf("%s \t", functions[num_col*j+i].key);
			++i;
		}
		printf("\n");
		++j;
	}
	size_t d = functions_table_size - num_col*j;
	i = 0;
	while (i < d) {
		printf("%s \t", functions[num_col*j+i].key);
		++i;
	}
	printf("\n");

}

CMD_SUB help_constants() {

	// extern size_t constants_table_size
	printf("\nBuilt-in constants (scientific constants are in SI units):\n\nkey\tvalue\n");
	size_t i = 0;
	while (i < constants_table_size) {
		printf(" %s \t %8.8g\n", constants[i].key, (double)constants[i].value);
		++i;
	}
	printf("\nUser-defined constants (variables) can be added with the command \"my <var-name> = <value>\".\n\n");

}

CMD_SUB help_my() {
	printf("\n\
The \"my\" command can be used to add user-defined constants to the program, i.e. to associate\n\
a string literal, either a single character or an arbitrarily long string, with a particular\n\
(decimal) value. However, some naming restrictions apply: pre-defined (builtin) constants or\n\
function names can not be overridden (see \"help constants/functions\" for a list of reserved keywords).\n\
Also, the identifier shouldn't begin with a digit.\n\
\n\
The lifetime of a variable added with \"my\" is until the session's termination.\n\
\n\
Usage: my <var-name> = <value>.\n\
\n\
Example:\n\
\n\
my x = sin((3/4)pi)\n\
\n");
}

CMD_SUB help_set() {
	printf("\n\
The \"set\" command can be used to control calc's inner workings. Available subcommands are:\n\
	set precision <integer argument>, controls how many decimals are shown in the final result\n\
\n");
}

CMD_MAIN set(word_list *wlist) {
	if (wlist->num_words < 2) {
		printf("\nset: error: expected subcommand, none supplied\n");
		return;
	}

	char *subcommand = wlist_get(wlist, 1);
	if (strcmp(subcommand, "precision") == 0) {
		if (wlist->num_words < 3) { 
			printf("\nset precision: error: argument expected.\n");
			return;
		}
		char* precstring = wlist_get(wlist, 2);
		_double_t prec_val = to_double_t(precstring);
		set_precision((int)prec_val);
	}

}

CMD_SUB set_precision(int precision) {
	if (precision < 1) { printf("set precision: error: precision requested < 1\n"); return; }
	if (precision > DEFAULT_PREC) { 
		printf("set precision: \033[1;31mwarning: incorrect decimals will almost certainly be included (p > %d)\033[m\n", DEFAULT_PREC); 
		if (precision > 56) { precision = 50; printf("Clamped precision to 50 decimal places.\n"); }
	}
	memset(f_precision, 0, PRECSTRING_MAX);	// perhaps a bit superfluous

#ifdef LONG_DOUBLE_PRECISION
	sprintf(f_precision, "= \033[1;29m%%.%dLg\033[m\n", precision);
#else
	sprintf(f_precision, "= \033[1;29m%%.%dg\033[m\n", precision);
#endif
}

CMD_MAIN quit() {
	udctree_delete();
	quit_signal = 1;
}

const key_funcptr_pair commands[] = {
{ "help", help },
{ "my", my },
{ "quit", quit },
{ "set", set }
};


const size_t commands_list_size = sizeof(commands)/sizeof(commands[0]); 



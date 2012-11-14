#include "commands.h"

extern const key_mathfuncptr_pair functions[];
extern const size_t functions_table_size;
extern const key_constant_pair constants[];
extern const size_t constants_table_size;

int quit_signal = 0;

static void my_list() {
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

static void help(word_list *wlist) {

	if (wlist->num_words == 1) {
	printf(
"\ncalc is a calculator application, meaning it parses\n\
the input mathematical expression and computes the answer.\n\
\n\
GNU readline(-style) line-editing and input history is supported.\n\
\n\
Try \"help functions\" for a list of supported functions,\n\
    \"help constants\" for a list of built-in constants,\n\
    \"help my\" for information on the \"my\" command (used to define custom variables).\n\
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
			else { 
				printf("\nhelp: unknown help index \"%s\".\n", keyword);
			}
		}
		else { printf("help: trailing character(s) (\"%s...\")\n", wlist_get(wlist,2)); }
	}

}

static void help_functions() {

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

static void help_constants() {

	// extern size_t constants_table_size
	printf("\nBuilt-in constants (scientific constants are in SI units):\n\nkey\tvalue\n");
	size_t i = 0;
	while (i < constants_table_size) {
		printf(" %s \t %8.8g\n", constants[i].key, (double)constants[i].value);
		++i;
	}
	printf("\nUser-defined constants (variables) can be added with the command \"my <var-name> = <value>\".\n\n");

}

static void help_my() {
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

static int clashes_with_predefined(const char* arg) {
	int i = 0;

//	extern const size_t constants_table_size;

	while (i < constants_table_size) {
		if (strcmp(constants[i].key, arg) == 0) {
			printf("my: error: operand varname clashes with predefined \"%s\" (= %f)\n", constants[i].key, (double)constants[i].value);
			return 1;
		}
		++i;
	}
	return 0;
}

void my(word_list *wlist) {
	// add variable to user-defined list (udc_list)
	// check that the my-invocation was properly formatted
	
	if (wlist->num_words > 1) {
		if (strcmp(wlist_get(wlist, 1), "list") == 0) {
			my_list();			
		}
		else {
			size_t recomposed_length;
			char *recomposed = wlist_recompose(wlist, &recomposed_length);
			int i = 3;	// the first 3 characters in the string are guaranteed to be { 'm', 'y', ' ' } 
			while (i < recomposed_length) {
				if (recomposed[i] == '=') { break; }
				++i;
			}
			if (i == recomposed_length) {
				free(recomposed);
				printf("Ill-formatted \"my\" invocation. See \"help my\".\n");
			}
			else {
				// we now know that the var-name is found at substring [3->i], and that the
				// value is found at [(i+1) -> recomposed_length]

				const size_t varname_end_pos = i-1;
				const size_t valstring_beg_pos = i+1;
				char *varname = substring(recomposed, 3, varname_end_pos-2);
				char *varname_stripped = strip_surrounding_whitespace_k(varname, strlen(varname));
				char *valstring = substring(recomposed, valstring_beg_pos, recomposed_length - valstring_beg_pos);
				char *valstring_stripped = strip_surrounding_whitespace_k(valstring, strlen(valstring));

				// differential diagnosis X-DD
				if (!valstring_stripped || !varname_stripped) { 
					printf("my: error: ill-formatted invocation. See \"help my\".\n");
					goto cleanup;
				}
				else if (is_digit(varname_stripped[0])) {
					printf("\nmy: error: variable identifiers cannot begin with digits.\n");
					goto cleanup;
				}
				else if (clashes_with_predefined(varname_stripped)) {
					// the call prints its own error message
					goto cleanup;
				}
				else {
					// check if already exists in the udctree.
					udc_node *search_result = udctree_search(varname_stripped);
					if (!search_result) {
						udc_node *newnode; 
						newnode = malloc(sizeof(udc_node)); 
						newnode->pair.key = strdup(varname_stripped);
						newnode->pair.value = parse_mathematical_input(valstring_stripped);
						udctree_add(newnode);
					}
					else {
						search_result->pair.value = parse_mathematical_input(valstring_stripped);
					}
				}

				cleanup:
					free(varname);
					free(varname_stripped);
					free(valstring);
					free(valstring_stripped);
			}
			free(recomposed);
		}
	}
	else {
		printf("\nmy: missing operand - see \"help my\".\n");
	}

}

void quit() {
	udctree_delete();
	quit_signal = 1;
}

const key_funcptr_pair commands[] = {
{ "help", help },
{ "my", my },
{ "quit", quit }

};

const size_t commands_list_size = sizeof(commands)/sizeof(commands[0]); 



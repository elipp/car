#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdlib.h>
#include <stdio.h>
#include "definitions.h"
#include "functions.h"
#include "utils.h"
#include "ud_constants_tree.h"

#define CMD_MAIN static void
#define CMD_SUB static void

CMD_MAIN help(word_list*);
CMD_SUB help_functions();
CMD_SUB help_constants();
CMD_SUB help_my();

CMD_MAIN set(word_list*);
CMD_SUB set_precision();

CMD_MAIN my(word_list*);
CMD_SUB my_list();

CMD_MAIN quit();

#endif

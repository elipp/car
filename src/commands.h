#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdlib.h>
#include <stdio.h>
#include "definitions.h"
#include "functions.h"
#include "utils.h"
#include "ud_constants_tree.h"

static void help(word_list*);
static void help_functions();
static void help_constants();
static void help_my();
void my(word_list*);

#endif

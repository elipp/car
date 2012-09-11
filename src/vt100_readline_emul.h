#ifndef VT100_READLINE_EMUL_H
#define VT100_READLINE_EMUL_H

/* readline emulation for vt100-compatible terminal emulators 
   (to remove the GPL'd GNU readline dependency) */

/* THIS IS NOT A FORK OF THE GNU READLINE LIBRARY, ONLY PROVIDES 
 * A SMALL SUBSET OF THE LINE-EDITING FUNCTIONALITY PROVIDED BY READLINE. */


#include <string.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>


// these two are really the only features we need
void e_readline_init();

char *e_readline(const char* prompt);
void e_history_add(const char *term);

#endif

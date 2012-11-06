#ifndef VT100_READLINE_EMUL_H
#define VT100_READLINE_EMUL_H

/* readline emulation for vt100-compatible terminal emulators 
   (to remove the GPL'd GNU readline dependency) */

/* THIS IS NOT A FORK OF THE GNU READLINE LIBRARY, ONLY IMPLEMENTS 
 * A SMALL SUBSET OF THE LINE-EDITING FUNCTIONALITY PROVIDED IN READLINE. */

#include <string.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>

// these two are really the only features we need
void e_readline_init();
void e_readline_deinit();

char *e_readline();
void e_hist_add(const char *term);

#endif

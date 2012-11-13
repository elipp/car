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

#include <sys/time.h>	// for select()
#include <sys/types.h>
#include <unistd.h>


#define IS_REGULAR_INPUT(x) ((x) > 0x19 && (x) < 0x7F)

// these are really the only two features we need
void e_readline_init();
void e_readline_deinit();

char *e_readline();
void e_hist_add(const char *term);

#endif

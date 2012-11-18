#ifndef RL_EMUL_H
#define RL_EMUL_H

/* readline emulation for ANSI X3.64-compatible terminal emulators 
   (to remove the GPL'd GNU readline dependency) 
   - emulation for emulators. :() */

/* THIS IS NOT A FORK OF THE GNU READLINE LIBRARY, ONLY IMPLEMENTS 
 * A SMALL SUBSET OF THE LINE-EDITING FUNCTIONALITY PROVIDED THEREIN. */

#include <string.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>

#define IS_REGULAR_INPUT(x) ((x) > 0x19 && (x) < 0x7F)

void rl_emul_init();
void rl_emul_deinit();

// these are really the only two *features* we need

char *rl_emul_readline(const char* prompt);
void rl_emul_hist_add(const char *term);

#endif

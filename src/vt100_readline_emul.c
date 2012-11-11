#include "vt100_readline_emul.h"

#define TMPBUF_SZ 1024

#define DECREMENT_CUR_POS_NZ()\
	do {\
		if (cur_pos > 0) { --cur_pos; }\
	} while(0)

#define DECREMENT_LINE_LEN_NZ() \
	do {\
		if (line_len > 0) { --line_len; }\
	} while(0)

#define INCREMENT_MARKERS()\
	do {\
		++cur_pos;\
		++line_len;\
	} while(0)

/* the following macros are debugging utility macros. */

#define BUFFER_PRINT_RAW_CHARS(n)\
	do {\
		printf("\nraw buffer contents up to %d: ", (n));\
		int i = 0;\
		while (i < (n)) {\
			if (buffer[i] == '\0') { putchar('@'); }\
			else putchar(buffer[i]);\
			++i;\
		}\
	} while(0)

#define PRINT_RAW_CHARS(buf, n)\
	do {\
		printf("\nraw buffer contents up to %lu: ", (n));\
		int i = 0;\
		while (i < (n)) {\
			if ((buf)[i] == '\0'){ putchar('@'); }\
			else putchar((buf)[i]);\
			++i;\
		}\
	} while(0)


#define HIST_CURRENT_INCREMENT()\
	do {\
		hist_current = hist_current->next ? hist_current->next : &buffer_current;\
	} while(0)

#define HIST_CURRENT_DECREMENT()\
	do {\
			hist_current = hist_current->prev ? hist_current->prev : hist_root;\
	} while(0)


#define CTRL_A		0x1
#define CTRL_E		0x5
#define CTRL_K		0xB
#define LINE_FEED 	'\n'
#define TAB_STOP	'\t'
#define BACKSPACE	0x7F

// vt100 arrow keypress control sequence (0x1B5Bxx) last byte identifiers

#define ARROW_UP 	0x41
#define ARROW_DOWN 	0x42	
#define ARROW_LEFT 	0x44
#define ARROW_RIGHT 	0x43	

#define DELETE		0x33

// function declarations

static void gb_create_gap(char* buffer);
static void gb_merge(char* buffer);

static const char* hist_get_current(size_t*);

typedef struct _hist_node {
	struct _hist_node *next;
	struct _hist_node *prev;
	char* line_contents;
	size_t line_length;
} hist_node;

// the hist_ is essentially a doubly-linked list of strings.
static hist_node buffer_current = { NULL, NULL, NULL, 0 }; // this node is always on top of the list :P
static hist_node *hist_current = &buffer_current;
static hist_node *hist_head = &buffer_current;
static hist_node *hist_root = &buffer_current;

static size_t hist_size = 0;

static void e_hist_destroy();

void e_hist_add(const char *arg) {
	
	if (!arg) { return; }

	const size_t arg_len = strlen(arg);
	hist_node *newnode = malloc(sizeof(hist_node));
	newnode->line_contents = strndup(arg, arg_len);
	newnode->line_length = arg_len;

	if (hist_head == &buffer_current) {
		hist_head = newnode;
		hist_root = newnode;
		hist_head->next = NULL;
		hist_head->prev = NULL;
	}
	else {
		hist_head->next = newnode;	// this also sets hist_root->next = newnode, when hist_root == hist_head
		newnode->prev = hist_head;
		newnode->next = NULL;
		hist_head = newnode;
	}

	hist_head->next = &buffer_current;
	buffer_current.prev = hist_head;
	buffer_current.next = NULL;
	hist_current = &buffer_current;
	++hist_size;
}

static void e_hist_destroy() {
	// free node by node :P
	hist_node *iter = hist_root;
	while (iter != NULL && iter != &buffer_current) {
		hist_node *nexttmp = iter->next;
		free(iter->line_contents);
		free(iter);
		iter = nexttmp;
	}
}

static const char* hist_get_current(size_t *histlen_ret) {
	*histlen_ret = hist_current ? hist_current->line_length : 0;
	return hist_current->line_contents;
}

/*
static void hist_print_entries() {
	printf("\nHistory entries:\n");
	hist_node *iter = NULL;
	for (iter = hist_root; iter != NULL; iter = iter->next) {
		printf("%s\n", iter->line_contents);
	}

} */

// anything prefixed with gb_ is related to the line-editing [g]ap [b]uffer

static enum { GB_NOEXIST = 0, GB_MIDLINE_INSERT, GB_MIDLINE_BACKSPACE} gb_exists;

static char *gb_pre = NULL;	// if gb_exists, points to cur_pos + 1
static char *gb_post = NULL;	// points to the first character after the gap.

static const size_t gap_width = 0xF;
	
static size_t cur_pos = 0;
static size_t line_len = 0; // this represents the net length of the line (i.e., not including the potential gap in the buffer)

static size_t post_len = 0;	// represents the length of the post-gap portion of the line.

static struct termios old;

void e_readline_init() {

	// to enable unbuffered stdin/out with getchar/putchar 
	struct termios new;
	tcgetattr(0, &old);
	new = old;
	new.c_lflag &=(~ICANON & ~ECHO);
	tcsetattr(0, TCSANOW, &new);

}

void e_readline_deinit() {
	tcsetattr(0, TCSANOW, &old);
	e_hist_destroy();
}

static void gb_create_gap(char *buffer) {
	// check for buffer overflow, lolz
	
	// create gap at cur_pos -> cur_pos + gap_width

	if (!gb_exists) {
		post_len = line_len - cur_pos;
		gb_pre = buffer + cur_pos + 1;	// by definition, points to cur_pos + 1
	}

	// relocate post-part of buffer
	
	char posttmp[post_len+1];	// we could store posttmp in a static, global variable though

	memcpy(posttmp, gb_exists ? gb_post : buffer + cur_pos, post_len);
	posttmp[post_len] = '\0';

	gb_post = gb_pre + gap_width;

	// the gap could be filled with debug characters for the time being (memset(gb_pre, '!', gap_width)), for instance
	memcpy(gb_post, posttmp, post_len);

	*(gb_pre) = '\0';
	gb_exists = GB_MIDLINE_INSERT;

}

static void gb_merge(char *buffer) {
	// assuming gb_pre & gb_post have meaningful values
	// copy post-part of buffer to position gb_pre
	
	char *posttmp = strndup(gb_post, post_len);	// will leak memory if not freed
	memcpy(gb_pre-1, posttmp, post_len);
	*(gb_pre+post_len-1) = '\0';
	// gap no longer exists, reset flag
	gb_exists = GB_NOEXIST;
	free(posttmp);
}


char *e_readline() {

	static const char* esc_cur_1_left = "\033[1D";
	static const char* esc_cur_1_right = "\033[1C";
	static const char* esc_cur_save = "\0337";
	static const char* esc_cur_restore = "\0338";
	static const char* esc_clear_cur_right = "\033[0K";
	static const char* esc_composite_bkspc = "\033[1D \033[1D";
	static const char* esc_cur_reset_left = "\r";

	static const char* esc_composite_clear_line_reset_left = "\r\033[0K";

	//printf("%s%s%s%s%s", esc_composite_bkspc, esc_cur_save, esc_clear_cur_right, gb_post, esc_cur_restore);
	static const char* esc_composite_ml_bk_pre = "\033[1D \033[1D\0337\033[0K";
	
	static char buffer[TMPBUF_SZ];
	memset(buffer, 0, sizeof(buffer));
	gb_pre = buffer;
	gb_post = buffer;

	cur_pos = 0;
	line_len = 0;
	char ctrl_char_buf[2];

	while (1) {
		char c;
		c = getchar();
		if (c == LINE_FEED) {	// enter, or line feed
			// if there's a gap in the buffer, merge
			if (gb_exists) { gb_merge(buffer); }
			putchar(c);
			break;
		}
		else if (c == TAB_STOP) {
			// do nothing :P
			// the real readline library has some autocompletion features
		}
		else if (c == CTRL_A) {
			printf("%s", esc_cur_reset_left);
			cur_pos = 0;
		}
		else if (c== CTRL_E) {
			printf("%s", esc_cur_reset_left);
			printf("\033[%dC", line_len);
			cur_pos = line_len;
		}
		else if (c == CTRL_K) {
			printf("%s", esc_clear_cur_right);
			buffer[cur_pos] = '\0';
		}
		else if (c == BACKSPACE) {	// backspace
			
			if (gb_exists == GB_MIDLINE_INSERT) {	
				gb_merge(buffer);	// this sets gb_exists = NOEXIST
			}

			if (cur_pos != line_len && !gb_exists) {
				gb_pre = buffer+cur_pos+1;	// this will get decremented at the end of this block
				gb_post = buffer+cur_pos;	// seems absurd, see above comment
				gb_exists = GB_MIDLINE_BACKSPACE;
				post_len = line_len - cur_pos;
			}
			// the printing code needs special attention if cur_pos != line_len

			if (cur_pos != line_len) {	// this pretty much guarantees gb_post has a meaningful value
				printf("%s%s%s", esc_composite_ml_bk_pre, gb_post, esc_cur_restore);
			} else {
				printf("%s", esc_composite_bkspc);
			}

			DECREMENT_CUR_POS_NZ();
			DECREMENT_LINE_LEN_NZ();
			buffer[cur_pos] = '\0';
			--gb_pre;
		}

		else if (c == '\033') {	// a.k.a. ascii 27; in this applicated mainly for handling arrowkey presses
			ctrl_char_buf[0] = getchar();	
			ctrl_char_buf[1] = getchar();

			static const char* hist_line = NULL;
			static size_t hist_line_len = 0;
			// could just give the ctrl_char_buf to printf
			if (ctrl_char_buf[0] != 91) { break; }
			switch (ctrl_char_buf[1]) {
				case ARROW_LEFT:
					if (gb_exists) {
						gb_merge(buffer);
					}
					printf("%s", esc_cur_1_left);
					DECREMENT_CUR_POS_NZ();
					break;
				case ARROW_RIGHT:
				
					if (gb_exists) {
						gb_merge(buffer);
					}
					if (cur_pos < line_len) {
						printf("%s", esc_cur_1_right);
						++cur_pos; 
					}
					break;
				case ARROW_UP:
					// clear whole line and buffer
					printf("%s", esc_composite_clear_line_reset_left);
					// to enable real readline-like behavior, the current buffer
					// is stored at the first position.
					if (hist_current == &buffer_current) {
						if (gb_exists) { gb_merge(buffer); }
						if (buffer_current.line_contents) { free(buffer_current.line_contents); }
						buffer_current.line_contents = strndup(buffer, line_len);
						buffer_current.line_length = line_len;
					}

					HIST_CURRENT_DECREMENT();

					hist_line = hist_get_current(&hist_line_len);
					if (hist_line) {
						line_len = hist_line_len;
						cur_pos = line_len;
						memcpy(buffer, (const void*)hist_line, hist_line_len);
						buffer[hist_line_len] = '\0';	// just to be sure :P
					} 
						
					printf("%s", buffer);

					break;

				case ARROW_DOWN:
					HIST_CURRENT_INCREMENT();
					printf("%s", esc_composite_clear_line_reset_left);

					if (hist_current == &buffer_current) {	
						hist_line = buffer_current.line_contents;
						line_len = buffer_current.line_length;
						cur_pos = line_len;
						memcpy(buffer, (const void*)buffer_current.line_contents, buffer_current.line_length);
						buffer[buffer_current.line_length] = '\0';	
					}
					else {
						hist_line = hist_get_current(&hist_line_len);
						line_len = hist_line_len;
						cur_pos = line_len;
						memcpy(buffer, (const void*)hist_line, hist_line_len);
						buffer[hist_line_len] = '\0';	
					}

					printf("%s", buffer);

					break;
				case DELETE:
					// the delete doesn't use gap buffering.
					// (would require an infrastructural overhaul)
					getchar();	// discard the '~' (0x7F) character
					if (gb_exists) {
						gb_merge(buffer);
					} 
					if (cur_pos < line_len) {	// there can only be mid-line deletes :P
						const size_t rem_len = line_len - cur_pos;
						gb_pre = buffer+cur_pos;
						gb_post = buffer+cur_pos+1;
						buffer[cur_pos] = '\0';
						printf("%s%s%s%s", esc_cur_save, esc_clear_cur_right, gb_post, esc_cur_restore);
						memcpy(gb_pre, gb_post, rem_len);
						DECREMENT_LINE_LEN_NZ();
					}
					break;
				default:
					break;
			}
		}

		else { 	/* regular character input */

			if (cur_pos != line_len && !gb_exists) {
				gb_create_gap(buffer);
				buffer[cur_pos] = c;
				++gb_pre;
				INCREMENT_MARKERS();
				putchar(c);
				printf("%s", esc_cur_save);
				printf("%s%s", gb_post, esc_cur_restore); 
			}
			else { 
				if (gb_exists) {
					if (gb_pre >= gb_post-1) {
						gb_create_gap(buffer);
					}
					++gb_pre;
				}

				putchar(c); 

				if (gb_exists) {
					// it's probably too risky to put a curpos_save and a restore on the same printf call
					printf("%s", esc_cur_save);
					printf("%s%s", gb_post, esc_cur_restore);
				}

				buffer[cur_pos] = c;
				INCREMENT_MARKERS();
				buffer[cur_pos] = '\0';
			}
		}
//		BUFFER_PRINT_RAW_CHARS(75);
	}
	
	if (line_len== 0) { return NULL; }
	else return strndup(buffer, line_len);

}




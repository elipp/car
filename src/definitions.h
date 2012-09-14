#ifndef DEFINITIONS_H
#define DEFINITIONS_H 

#include <stdlib.h>

#define PRIO_ADD_SUB 0x0
#define PRIO_MUL_DIV 0x1
#define PRIO_POW_FAC 0x2

#define PARSE_NOT_REQUIRED 0x00
#define PARSE_REQUIRED 0x01

#ifdef LONG_DOUBLE_PRECISION
typedef long double _double_t;
#else
typedef double _double_t;
#endif 

typedef struct {
	const char* key;
	_double_t (*funcptr)(_double_t);
} key_mathfuncptr_pair;

typedef struct {
	char* key;
	_double_t value;
} key_constant_pair;

typedef struct {
	const char* key;
	_double_t (*funcptr)(const char*);
} key_strfuncptr_pair;

// udc. user-defined constant
typedef struct _word_node {
	struct _word_node *next;
	char* text;
	size_t length;
} word_node;

typedef struct {
	word_node *head;
	word_node *root;
	size_t num_words;
} word_list;

typedef struct {
	const char *key;
	void *(*func)(void*);	// the argument is always either a NULL or a wlist*
} key_funcptr_pair;


_double_t func_pass_get_result(const char*, size_t, int*);
_double_t constant_pass_get_result(const char*, size_t);

#endif

#ifndef DEFINITIONS_H
#define DEFINITIONS_H 

#include <stdlib.h>

#define OP_NOP 0x0
#define OP_ADD '+'
#define OP_SUB '-'
#define OP_MUL '*'
#define OP_DIV '/'
#define OP_POW '^'

#define PARAM_FIRST 0xF
#define PARAM_NOP 0x0
#define PARAM_SGN_POS 0x1
#define PARAM_SGN_NEG 0x2
#define PARAM_MUL 0x3
#define PARAM_DIV 0x4
#define PARAM_POW 0x5
#define PARAM_FAC 0x6

#define PRIO_ADD_SUB 0x0
#define PRIO_MUL_DIV 0x1
#define PRIO_POW_FAC 0x2

#define PARSE_NOT_REQUIRED 0x00
#define PARSE_REQUIRED 0x01

// see tree_generate
#define MATCHES_WITH_OPERATOR(x) (x == operators[0] || x == operators[1])

#ifdef LONG_DOUBLE_PRECISION
typedef long double _double_t;
#else
typedef double _double_t;
#endif 

typedef struct {
	const char* key;
	_double_t (*funcptr)(_double_t);
} key_funcptr_pair;

typedef struct {
	const char* key;
	_double_t value;

} key_constant_pair;

_double_t func_pass_get_result(const char*, size_t, int*);
_double_t constant_pass_get_result(const char*, size_t);

#endif

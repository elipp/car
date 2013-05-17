#ifndef OBJLOADER_H
#define OBJLOADER_H

#define LINEBUF_MAX 128

#include <fstream>
#include "vertex.h"

enum offsetfileinfo { O_VERTICES=0, O_NORMALS, O_TEXCOORDS, O_QUADFACES, O_TRIANGLEFACES };	// to be used with the objinfo info[5] structure

enum fileextensions { OBJ=0, BOBJ, CBOBJ };

enum {		// trailing O stands for "offset", trailing C for "count".
	VO=0,	
	VC,
	NO,
	NC,
	TO,
	TC,
	QFO,
	QFC,
	TFO,
	TFC 
};

enum {
	BVC=0,	// BINARY VERTEX COUNT
	BNC,	// - - -  NORMAL - - -
	BTC,	// - - -  TEXCRD - - -
	BQFC,	// - - -  QFACES - - -
	BTFC	// - - -  TFACES - - -
};

enum {
	
	TRIANGLES=3,
	QUADS=4
	
};


static const int bobj = 0x6a626f62;



bool checkext(const char*, int);
bool headerValid(char *);

int loadObj(const char*);	// for backwards compatibility, not vertex*

unsigned short int *loadBObj(const char* filename, bool compressed, GLuint* VBOid, GLuint *facecount);		// returns index buffer
unsigned short int *loadNewBObj(const char* filename, GLuint *VBOid, GLuint *facecount);
GLuint loadNewestBObj(const std::string &filename, GLuint *facecount);

size_t decompress(FILE *file, char** buffer);
std::size_t cpp_decompress(std::ifstream& file, char** buffer);
#endif
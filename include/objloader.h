#ifndef OBJLOADER_H
#define OBJLOADER_H

#define LINEBUF_MAX 128

#include <fstream>

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
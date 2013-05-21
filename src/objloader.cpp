#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "glwindow_win32.h"

#include "text.h"
#include "vertex.h"
#include "objloader.h"
#include "common.h"


GLuint loadNewestBObj(const std::string &filename, GLuint *facecount) {

	std::ifstream infile(filename.c_str(), std::ios::binary | std::ios::in);
	
	if (!infile.is_open()) { onScreenLog::print( "loadNewestBObj: failed loading file %s\n", filename.c_str()); return NULL; }
	
	onScreenLog::print("loadNewestBObj: loading file %s\n", filename.c_str());

	std::size_t filesize = cpp_getfilesize(infile);
	char *buffer = new char[filesize];

	infile.read(buffer, filesize);
	
	char* iter = buffer + 4;	// the first 4 bytes of a bobj file contain the letters "bobj". Or do they? :D

	// read vertex & face count.

	unsigned int vcount = *((unsigned int*)iter);
	*facecount = (GLuint)(vcount/3);

	iter += 4;

	vertex* vertices = new vertex[vcount];

	iter = buffer + 8;

	memcpy(vertices, iter, vcount*8*sizeof(float));

	GLuint VBOid;

	glGenBuffers(1, &VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, VBOid);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*vcount, &vertices[0].vx, GL_STATIC_DRAW);

	//printf("%f %f %f", vertices[0].vx, vertices[0].vy, vertices[0].vz);

	return VBOid;


}

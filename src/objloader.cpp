#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "glwindow_win32.h"

#include "text.h"
#include "vertex.h"
#include "objloader.h"
#include "common.h"

#include <cassert>

#include "lzma/mylzma.h"

extern GLuint IBOid;
extern mat4 projection;

#define NUM_FACES_PER_VBO_MAX ((0xFFFF)/3)


Model::Model(const std::string &filename, ShaderProgram *const prog) : id_string(filename), program(prog) {

	_bad = true;
	has_texture = false;

	ModelView = mat4::identity();

	/*std::ifstream infile(filename, std::ios::binary | std::ios::in);
	
	if (!infile.is_open()) { 
		PRINT "Model: failed loading file %s\n", filename.c_str()); 
		_bad = true;
		return;
	}*/
	
	PRINT(".bobjloader: loading file %s\n", filename.c_str());

	char *decompressed;
	size_t decompressed_size;

	int res = LZMA_decode(filename, &decompressed, &decompressed_size);	// this also allocates the memory
	if (res != 0) {
		PRINT("LZMA decoder: an error occurred. Aborting.\n"); 
		_bad = true; 
		return; 
	}

	char* iter = decompressed + 4;	// the first 4 bytes of a bobj file contain the letters "bobj". Or do they? :D

	// read vertex & face count.

	unsigned int vcount;
	memcpy(&vcount, iter, sizeof(vcount));

	num_faces = vcount / 3;
	
	vertex* vertices = new vertex[vcount];
	
	iter = decompressed + 8;
	memcpy(vertices, iter, vcount*8*sizeof(float));
	delete [] decompressed;	// no longer needed
	
	glGenVertexArrays(1, &VAOid);
	glGenBuffers(1, &VBOid);

	glBindVertexArray(VAOid);

	glEnableVertexAttribArray(ATTRIB_POSITION);
	glEnableVertexAttribArray(ATTRIB_NORMAL);
	glEnableVertexAttribArray(ATTRIB_TEXCOORD);
	
	glBindBuffer(GL_ARRAY_BUFFER, VBOid);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*num_faces*3, vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(0));
	glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(3*sizeof(float)));
	glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(6*sizeof(float)));

	glBindVertexArray(0);
		
	glDisableVertexAttribArray(ATTRIB_POSITION);
	glDisableVertexAttribArray(ATTRIB_NORMAL);
	glDisableVertexAttribArray(ATTRIB_TEXCOORD);
		
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
		
	PRINT("Successfully loaded file %s.\n\n", filename.c_str(), vcount);
	delete [] vertices;
	
	_bad = false;
	
}

void Model::bind_texture(GLuint _texId) {
	has_texture = true;
	this->texId = _texId;
}

void Model::use_ModelView(const mat4 &mv) {
	ModelView = mv;
}

void Model::draw() {
	
	glUseProgram(program->getProgramHandle());

	if (has_texture) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texId);
		program->update_uniform_1i("texture_color", 0);
	}

	program->update_uniform_mat4("ModelView", this->ModelView);
	program->update_uniform_mat4("Projection", projection);

	glBindVertexArray(VAOid);
	glDrawArrays(GL_TRIANGLES, 0, num_faces*3);
	glBindVertexArray(0);
	
	
}



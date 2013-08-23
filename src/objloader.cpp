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
		fprintf(stderr, "Model: failed loading file %s\n", filename.c_str()); 
		_bad = true;
		return;
	}*/
	
	onScreenLog::print(".bobjloader: loading file %s\n", filename.c_str());

	char *decompressed;
	size_t decompressed_size;

	int res = LZMA_decode(filename, &decompressed, &decompressed_size);	// this also allocates the memory
	if (res != 0) {
		fprintf(stderr, "LZMA decoder: an error occurred. Aborting.\n"); 
		_bad = true; 
		return; 
	}

	char* iter = decompressed + 4;	// the first 4 bytes of a bobj file contain the letters "bobj". Or do they? :D

	// read vertex & face count.

	unsigned int vcount;
	memcpy(&vcount, iter, sizeof(vcount));

	unsigned total_facecount = vcount / 3;
	unsigned num_VBOs = total_facecount/NUM_FACES_PER_VBO_MAX + 1;
	unsigned num_excess_last = total_facecount%NUM_FACES_PER_VBO_MAX;

	vdata.VBOids.resize(num_VBOs);
	vdata.VAOids.resize(num_VBOs);
	vdata.num_faces.resize(num_VBOs);
	
	for (int i = 0; i < num_VBOs-1; ++i) {
		vdata.num_faces[i] = NUM_FACES_PER_VBO_MAX;
	}
	vdata.num_faces[num_VBOs-1] = num_excess_last; 
	glGenVertexArrays(num_VBOs, &vdata.VAOids[0]);
	glGenBuffers(num_VBOs, &vdata.VBOids[0]);

	vdata.size = num_VBOs;

	onScreenLog::print("# vcount = %u => faces = %u => num_VBOs = %u, num_excess_last = %u\n", vcount, total_facecount, num_VBOs, num_excess_last);
	
	vertex* vertices = new vertex[vcount];
	
	iter = decompressed + 8;
	memcpy(vertices, iter, vcount*8*sizeof(float));
	delete [] decompressed;	// no longer needed
	
	size_t offset = 0;
	for (int i = 0; i < num_VBOs; i++) {
		glBindVertexArray(vdata.VAOids[i]);

		glEnableVertexAttribArray(ATTRIB_POSITION);
		glEnableVertexAttribArray(ATTRIB_NORMAL);
		glEnableVertexAttribArray(ATTRIB_TEXCOORD);

		glBindBuffer(GL_ARRAY_BUFFER, vdata.VBOids[i]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*vdata.num_faces[i]*3, vertices + offset, GL_STATIC_DRAW);

		glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(0));
		glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(3*sizeof(float)));
		glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(6*sizeof(float)));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOid);
		glBindVertexArray(0);
		
		glDisableVertexAttribArray(ATTRIB_POSITION);
		glDisableVertexAttribArray(ATTRIB_NORMAL);
		glDisableVertexAttribArray(ATTRIB_TEXCOORD);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		offset += vdata.num_faces[i]*3;
	}
	
	onScreenLog::print("Successfully loaded file %s.\n\n", filename.c_str(), vcount);
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

	for (int i = 0; i < vdata.size; ++i) {
		glBindVertexArray(vdata.VAOids[i]);
		glDrawElements(GL_TRIANGLES, vdata.num_faces[i]*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 
		glBindVertexArray(0);
	}
	
}



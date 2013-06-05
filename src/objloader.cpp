#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "glwindow_win32.h"

#include "text.h"
#include "vertex.h"
#include "objloader.h"
#include "common.h"

extern GLuint IBOid;
extern mat4 projection;

#define NUM_FACES_PER_VBO_MAX ((0xFFFF)/3)


Model::Model(const std::string &filename, ShaderProgram *const prog) : id_string(filename), program(prog) {

	_bad = true;

	has_texture = false;

	ModelView = mat4::identity();

	std::ifstream infile(filename, std::ios::binary | std::ios::in);
	
	if (!infile.is_open()) { 
		fprintf(stderr, "Model: failed loading file %s\n", filename.c_str()); 
		_bad = true;
		return;
	}
	
	onScreenLog::print(".bobjloader: loading file %s\n", filename.c_str());

	std::size_t filesize = cpp_getfilesize(infile);
	char *buffer = new char[filesize];

	infile.read(buffer, filesize);
	
	char* iter = buffer + 4;	// the first 4 bytes of a bobj file contain the letters "bobj". Or do they? :D

	// read vertex & face count.

	unsigned int vcount;
	memcpy(&vcount, iter, sizeof(vcount));

	int total_facecount = vcount / 3;
	int num_VBOs = total_facecount/NUM_FACES_PER_VBO_MAX + 1;

	unsigned num_excess_last = total_facecount%NUM_FACES_PER_VBO_MAX;
	
	onScreenLog::print("# faces = %d => num_VBOs = %d, num_excess_last = %d\n", total_facecount, num_VBOs, num_excess_last);

	GLuint *VBOids = new GLuint[num_VBOs];
	glGenBuffers(num_VBOs, VBOids);
	unsigned i = 0;
	for (i = 0; i < num_VBOs-1; ++i) {
		VBOid_numfaces_map.insert(std::pair<GLuint, unsigned short>(VBOids[i], NUM_FACES_PER_VBO_MAX));
	}
	VBOid_numfaces_map.insert(std::pair<GLuint,unsigned short>(VBOids[i], num_excess_last));
	delete [] VBOids;

	vertex* vertices = new vertex[vcount];

	iter = buffer + 8;

	memcpy(vertices, iter, vcount*8*sizeof(float));
	size_t offset = 0;
	for (auto &iter : VBOid_numfaces_map) {
		const GLuint &current_VBOid = iter.first;
		const unsigned short &num_faces = iter.second;
		glBindBuffer(GL_ARRAY_BUFFER, current_VBOid);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*num_faces*3, &vertices[offset].vx, GL_STATIC_DRAW);
		offset += NUM_FACES_PER_VBO_MAX*3;
	}
	
	onScreenLog::print("Successfully loaded file %s.\n\n", filename.c_str(), vcount);
	delete [] vertices;
	_bad = false;

}

void Model::bind_texture(GLuint _texId) {
	has_texture = true;
	this->texId = _texId;
}

void Model::use_ModelView(const mat4 &mw) {
	ModelView = mw;
}

void Model::draw() {
	
	glUseProgram(program->getProgramHandle());

	if (has_texture) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texId);
		program->update_uniform_1i("texture_color", 0);
	}

	program->update_uniform_mat4("ModelView", this->ModelView.rawData());
	program->update_uniform_mat4("Projection", projection.rawData());

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOid);  // is still in full matafaking effizzect :D 
	for (auto &iter : VBOid_numfaces_map) {

		const GLuint &current_VBOid = iter.first;
		const unsigned short &num_faces = iter.second;
		
		glBindBuffer(GL_ARRAY_BUFFER, current_VBOid);	
		glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(0));
		glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(3*sizeof(float)));
		glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(6*sizeof(float)));
	
		glDrawElements(GL_TRIANGLES, num_faces*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 
	}
	
}



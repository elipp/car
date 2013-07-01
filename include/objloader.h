#ifndef OBJLOADER_H
#define OBJLOADER_H

#define LINEBUF_MAX 128

#include <fstream>
#include <map>
#include "shader.h"
#include "lin_alg.h"

static const int bobj = 0x6a626f62;

__declspec(align(16))
class Model {
	mat4 ModelView;
	std::string id_string;
	std::map<GLuint, unsigned short> VBOid_numfaces_map;
	GLuint texId;
	ShaderProgram *program;
	bool has_texture;
	bool _bad;
public:
	Model(const std::string &filename, ShaderProgram * const prog);
	void draw();
	void bind_texture(GLuint texId);
	bool bad() const { return _bad; }
	void use_ModelView(const mat4 &mv);
	
	void *operator new(size_t size) {
		void *p;
#ifdef _WIN32
		p = _aligned_malloc(size, 16);
#elif __linux__
		posix_memalign(&p, 16, size);
#endif
		return p;
	}
	void *operator new[](size_t size) {
		void *p;
#ifdef _WIN32
		p = _aligned_malloc(size, 16);
#elif __linux__
		posix_memalign(&p, 16, size);
#endif
		return p;
	}

	void operator delete(void *p) {
#ifdef _WIN32
		_aligned_free(p);
#elif __linux__
		Model *q = static_cast<Model*>(p);	// the cast could be unnecessary
		free(q);
#endif
	}

	void operator delete[](void *p) {
#ifdef _WIN32
		_aligned_free(p);
#elif __linux__
		Model *q = static_cast<Model*>(p);	// the cast could be unnecessary
		free(q);
#endif
	}
};

bool checkext(const char*, int);
bool headerValid(char *);

int loadObj(const char*);	// for backwards compatibility, not vertex*

unsigned short int *loadBObj(const char* filename, bool compressed, GLuint* VBOid, GLuint *facecount);		// returns index buffer
unsigned short int *loadNewBObj(const char* filename, GLuint *VBOid, GLuint *facecount);
GLuint loadNewestBObj(const std::string &filename, GLuint *facecount);

size_t decompress(FILE *file, char** buffer);
std::size_t cpp_decompress(std::ifstream& file, char** buffer);
#endif
#ifndef MODEL_H
#define MODEL_H

#include "common.h"
#include "lin_alg.h"
#include <cstdlib>

#define ALIGNMENT 16

#ifdef _WIN32
__declspec(align(ALIGNMENT)) // to ensure 16-byte alignment in memory
#endif
class Model {

public:	
	// apparently, the default allocator can still return misaligned addresses, need to overload
	mat4 model_matrix;
	Quaternion rotation;
	vec4 position;
	vec4 velocity;
	
	float scale;
	float radius;

	float mass;
	//pyorimismaara? tensori :D:D
	
	Model(float mass, float scale, GLuint VBOid, GLuint texId, GLuint facecount, bool lightsrc, bool _fixed_pos = false);

	GLuint getVBOid() const { return VBOid; }
	GLuint getTextureId() const { return textureId; }
	GLuint getFaceCount() const { return facecount; }
	bool lightsrc() const { return is_a_lightsrc; }


	void translate(const vec4 &vec);
	void updatePosition();

	void *operator new(std::size_t size)  { 
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
		Model* M = static_cast<Model*>(p);
		free(M);
#endif

	}
	
private:
	GLuint VBOid;
	GLuint textureId;
	GLuint facecount;
	bool is_a_lightsrc;
	bool fixed_pos;

}
#ifdef __linux__
__attribute__((aligned(ALIGNMENT)))
#endif
;

#endif

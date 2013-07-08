#ifndef MODEL_H
#define MODEL_H

#include "common.h"
#include "lin_alg.h"
#include <cstdlib>

_BEGIN_ALIGN16
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

	_DEFINE_ALIGNED_MALLOC_FREE_MEMBERS;
	
private:
	GLuint VBOid;
	GLuint textureId;
	GLuint facecount;
	bool is_a_lightsrc;
	bool fixed_pos;

} _END_ALIGN16;

#endif

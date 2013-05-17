#include "model.h"

Model::Model(float m, float s, GLuint _VBOid, GLuint texId, GLuint _facecount, bool lightsrc, bool _fixed_pos) 
	: mass(m), scale(s), VBOid(_VBOid), facecount(_facecount), textureId(texId), is_a_lightsrc(lightsrc), fixed_pos(_fixed_pos) { 
	
		model_matrix = mat4::identity();
		model_matrix = scale*model_matrix;
		model_matrix(3, 3) = 1.0;
		// no need to initialize Quaternion rotation, because
		// default constructor initializes it to QUAT_NO_ROTATION.
		// The same applies to vec4, it's all zeroes by default.
		position(3) = 1.0;


		// SPHERE SPECIFIC

		radius = scale*0.92;
		
}


// weird function?

void Model::translate(const vec4 &vec) {

	if (!fixed_pos) {
		position += vec;
		model_matrix.assignToColumn(3, position);
	}
	
}

void Model::updatePosition() {

	model_matrix.assignToColumn(3, position);

}

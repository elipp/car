#ifndef OBB_H
#define OBB_H

#include "lin_alg.h"

class OBB {

	vec4 A0, A1, A2;	// orthonormal, right-handed axes
	vec4 e;	// corresponding extents.
	Quaternion rot_q;	// rotation quaternion, normalized
	vec4 C;	// center point

public:
	int collision_test_SAT(const OBB &b);
	int collision_test_GJK(const OBB &b);

};

#endif
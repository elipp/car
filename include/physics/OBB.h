#ifndef OBB_H
#define OBB_H

#include "lin_alg.h"

__declspec(align(16))
struct OBB {
	vec4 A0, A1, A2;	// orthonormal, right-handed axes. 
						// Always updated to reflect the rotation of the host body.
	vec4 e;	// corresponding extents.
	Quaternion rot_q;	// rotation quaternion, normalized
	vec4 C;	// center point
};

int collision_test_SAT(const OBB &a, const OBB &b);
int collision_test_GJK(const OBB &a, const OBB &b);



#endif
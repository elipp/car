#ifndef OBB_H
#define OBB_H

#include "lin_alg.h"

__declspec(align(16))
struct OBB {
	vec4 A0, A1, A2;	// orthonormal, right-handed axes. 
						// Always updated to reflect the rotation of the host body.
	vec4 e;	// corresponding extents.
	vec4 C;	// center point
	void rotate(Quaternion q) {
		// this sux, tbh :P
		A0 = vec4(1.0, 0.0, 0.0, 0.0).applyQuatRotation(q);
		A1 = vec4(0.0, 1.0, 0.0, 0.0).applyQuatRotation(q);
		A2 = vec4(0.0, 0.0, 1.0, 0.0).applyQuatRotation(q);
	}
	OBB() {
		A0 = vec4(1.0, 0.0, 0.0, 0.0);
		A1 = vec4(0.0, 1.0, 0.0, 0.0);
		A2 = vec4(0.0, 0.0, 1.0, 0.0);
		C = vec4(0.0, 0.0, 0.0, 1.0);
	}

	OBB(const vec4 &extents) {
		A0 = vec4(1.0, 0.0, 0.0, 0.0);
		A1 = vec4(0.0, 1.0, 0.0, 0.0);
		A2 = vec4(0.0, 0.0, 1.0, 0.0);
		C = vec4(0.0, 0.0, 0.0, 1.0);
		e = extents;
	}
};

int collision_test_SAT(const OBB &a, const OBB &b);
int collision_test_GJK(const OBB &a, const OBB &b);



#endif
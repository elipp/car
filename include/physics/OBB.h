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
		A0 = vec4(1.0, 0.0, 0.0, 0.0).applyQuatRotation(q).normalized();
		A1 = vec4(0.0, 1.0, 0.0, 0.0).applyQuatRotation(q).normalized();
		A2 = vec4(0.0, 0.0, 1.0, 0.0).applyQuatRotation(q).normalized();
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
	void compute_box_vertices(vec4 *out8) const;
};

int collision_test_SAT(const OBB &a, const OBB &b);

__declspec(align(16))
struct GJKSession {
private:
	mat4 VAm1, VAm2, VBm1, VBm2;
	mat4 VAm1_T, VAm2_T, VBm1_T, VBm2_T;	// these are stored because column() is at least 51965964 times faster than row(). At least.
	vec4 support(const vec4 &D);
public:
	// don't like the interface though.
	GJKSession(const OBB &a, const OBB &b);
	int collision_test();
};


#endif
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
		const mat4 qm = q.toRotationMatrix();
		A0 = qm*vec4(1.0, 0.0, 0.0, 0.0).normalized();
		A1 = qm*vec4(0.0, 1.0, 0.0, 0.0).normalized();
		A2 = qm*vec4(0.0, 0.0, 1.0, 0.0).normalized();
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
struct mat4_doublet {
	mat4 m[2];
	mat4_doublet(const mat4 &m0, const mat4 &m1) { m[0] = m0; m[1] = m1; }
	mat4_doublet transposed_both() const {
		return mat4_doublet(m[0].transposed(), m[1].transposed());
	}
	mat4 &operator()(int n) { return m[n]; }
	vec4 column(int col) { return col > 3 ? m[1].column(col - 4) : m[0].column(col); }
	mat4_doublet() {};
};

__declspec(align(16))
struct GJKSession {
private:
	mat4_doublet VAm, VAm_T;
	mat4_doublet VBm, VBm_T;
	vec4 support(const vec4 &D);
public:
	// don't like the interface though.
	GJKSession(const OBB &a, const OBB &b);
	int collision_test();
};


#endif
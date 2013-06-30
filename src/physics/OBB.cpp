#include "physics/OBB.h"

__declspec(align(16)) 
struct float_arr_vec4 {
		float f[4];
		inline void operator=(const vec4 &v) {
			_mm_store_ps(f, v.getData());
		}
		float_arr_vec4(const vec4 &v) {
			*this = v;
		}
		inline float operator()(int col) const { return f[col]; }
};

__declspec(align(16))
struct float_arr_mat4 {
	float f[16];
	inline void operator=(const mat4 &m) {
		memcpy(f, m.rawData(), 16*sizeof(float));
	}
	float_arr_mat4(const mat4 &m) {
		*this = m;
	}

	inline float operator()(int col, int row) const { return f[4*col + row]; }
};

struct Simplex {
	vec4 points[4];
	int current_num_points;

	void add(const vec4 &p) {
		points[current_num_points] = p;
		++current_num_points;
	}
	void clear() {
		current_num_points = 0; // :D
	}
};



int collision_test_SAT(const OBB &a, const OBB &b) {

	// test for collision between two OBBs using the Separating Axis Theorem (SAT).
	// http://www.geometrictools.com/Documentation/DynamicCollisionDetection.pdf
	
	// assume axes reflect the orientation of the actual body (ie. have already been multiplied by the orientation quaternion)

	// this is an awful lot of initialization code, considering that the function could actually terminate on the first test :D
	// actual performance gains are observed maybe in the worst case (test #15)
	mat4 A(a.A0, a.A1, a.A2, vec4::zero4);
	mat4 A_T = A.transposed();
	const float_arr_vec4 a_e(a.e);

	mat4 B(b.A0, b.A1, b.A2, vec4::zero4);
	mat4 B_T = B.transposed();
	const float_arr_vec4 b_e(b.e);

	mat4 C = A_T * B;	// get c_ij coefficients
	const float_arr_mat4 C_f(C);

	mat4 absC = abs(C);
	mat4 absC_T = absC.transposed();
	const float_arr_mat4 absC_T_f(absC_T);

	vec4 D = b.C - a.C;	// vector between the two center points
	D.assign(V::w, 0.0);

	vec4 A_T_D = A_T*D;
	const float_arr_vec4 dp_Ai_D(A_T_D); // Ai · D
	const float_arr_vec4 abs_dp_Ai_D(abs(A_T_D)); // |Ai · D|
	const float_arr_vec4 abs_dp_Bi_D(abs(B_T*D)); // |Bi · D|

	// start testing for each of the 15 axes (R > R0 + R1). Reference PDF, Table 1 ^^

	float R, R0, R1;	// hopefully the compiler will optimize these away :P

	// L = Ai
	R0 = a_e(0);
	R1 = dot3(absC_T.column(0), b.e);	
	R = abs_dp_Ai_D(0);
	if (R > R0 + R1) {}

	R0 = a_e(1);
	R1 = dot3(absC_T.column(1), b.e);	
	R = abs_dp_Ai_D(1);
	if (R > R0 + R1) {}

	R0 = a_e(2);
	R1 = dot3(absC_T.column(2), b.e);
	R = abs_dp_Ai_D(2);
	if (R > R0 + R1) {}

	// L = Bi
	R0 = dot3(absC.column(0), a.e);
	R1 = b_e(0);
	R = abs_dp_Bi_D(0);
	if (R > R0 + R1) {}

	R0 = dot3(absC.column(1), a.e);
	R1 = b_e(1);
	R = abs_dp_Bi_D(1);
	if (R > R0 + R1) {}

	R0 = dot3(absC.column(2), a.e);
	R1 = b_e(2);
	R = abs_dp_Bi_D(2);
	if (R > R0 + R1) {}

	// various cross products
	
	// L = A0 x B0
	R0 = a_e(1)*absC_T_f(2, 0) + a_e(2)*absC_T_f(1,0);
	R1 = b_e(1)*absC_T_f(0, 2) + b_e(2)*absC_T_f(0,1);
	R = fabs(C_f(0,1)*dp_Ai_D(2) - C_f(0,2)*dp_Ai_D(1));
	if (R > R0 + R1) {}

	// L = A0 x B1
	R0 = a_e(1)*absC_T_f(2,1) + a_e(2)*absC_T_f(1,1);
	R1 = b_e(1)*absC_T_f(0,2) + b_e(2)*absC_T_f(0,0);
	R = fabs(C_f(1,1)*dp_Ai_D(2) - C_f(1,2)*dp_Ai_D(1));
	if (R > R0 + R1) {}

	// L = A0 x B2
	R0 = a_e(1)*absC_T_f(2,0) + a_e(2)*absC_T_f(1,0);
	R1 = b_e(1)*absC_T_f(0,1) + b_e(2)*absC_T_f(0,0);
	R = fabs(C_f(2,1)*dp_Ai_D(2) - C_f(2,2)*dp_Ai_D(1));
	if (R > R0 + R1) {}
	
	// L = A1 x B0
	R0 = a_e(0)*absC_T_f(2,0) + a_e(2)*absC_T_f(0,0);
	R1 = b_e(1)*absC_T_f(1,2) + b_e(2)*absC_T_f(1,1);
	R = fabs(C_f(0,2)*dp_Ai_D(0) - C_f(0,0)*dp_Ai_D(2));
	if (R > R0 + R1) {}

	// L = A1 x B1
	R0 = a_e(0)*absC_T_f(2,1) + a_e(2)*absC_T_f(0,1);
	R1 = b_e(0)*absC_T_f(1,2) + b_e(2)*absC_T_f(1,0);
	R = fabs(C_f(1,2)*dp_Ai_D(0) - C_f(0,0)*dp_Ai_D(2));
	if (R > R0 + R1) {}

	// L = A1 x B2
	R0 = a_e(0)*absC_T_f(2,2) + a_e(2)*absC_T_f(0,2);
	R1 = b_e(0)*absC_T_f(1,1) + b_e(1)*absC_T_f(1,0);
	R = fabs(C_f(2,2)*dp_Ai_D(0) - C_f(2,0)*dp_Ai_D(2));
	if (R > R0 + R1) {}
	
	// L = A2 x B0
	R0 = a_e(0)*absC_T_f(1,0) + a_e(1)*absC_T_f(0,0);
	R1 = b_e(1)*absC_T_f(2,2) + b_e(2)*absC_T_f(2,1);
	R = fabs(C_f(0,0)*dp_Ai_D(1) - C_f(0,1)*dp_Ai_D(0));
	if (R > R0 + R1) {}

	// L = A2 x B1
	R0 = a_e(0)*absC_T_f(1,1) + a_e(1)*absC_T_f(0,1);
	R1 = b_e(0)*absC_T_f(2,2) + b_e(2)*absC_T_f(2,0);
	R = fabs(C_f(1,0)*dp_Ai_D(1) - C_f(1,1)*dp_Ai_D(0));
	if (R > R0 + R1) {}

	// L = A2 x B2
	R0 = a_e(0)*absC_T_f(1,2) + a_e(1)*absC_T_f(0,2);
	R1 = b_e(0)*absC_T_f(2,1) + b_e(1)*absC_T_f(2,0);
	R = fabs(C_f(2,0)*dp_Ai_D(1) - C_f(2,1)*dp_Ai_D(0));
	if (R > R0 + R1) {}

	return 0;

}

// GJK (Gilbert-Johnson-Keerthi). References:
// http://mollyrocket.com/849 !!,
// http://www.codezealot.org/archives/88.


static vec4 GJK_support(const vec4 &D, const vec4 *const VA, const vec4 *const VB) {
	// returns a point within the Minkowski difference of the two boxes.
	// Basically, we find the vertex that is farthest away in the direction D among
	// the vertices of box A (VA), and the one farthest away in direction -D among
	// the vertices of box B (VB).

	const mat4 VAm1 = mat4(VA[0], VA[1], VA[2], VA[3]).transposed();
	const mat4 VAm2 = mat4(VA[4], VA[5], VA[6], VA[7]).transposed();

	vec4 dpVA1 = VAm1 * D;	// now we have the first four dot products in the vector
	vec4 dpVA2 = VAm2 * D;	// and the last four in another

	// find maximum dotp. just brute force for now :D
	int max_index_A1 = 0;
	int max_index_A2 = 0;
	float current_max_dp1 = dpVA1(0);
	float current_max_dp2 = dpVA2(0);

	for (int i = 1; i < 4; ++i) {
		float dp1 = dpVA1(i);
		if (dp1 > current_max_dp1) {
			max_index_A1 = i;
		}
		float dp2 = dpVA2(i);
		if (dp2 > current_max_dp2) {
			max_index_A2 = i;
		}
	}

	int max_dp_index_A = (dpVA1(max_index_A1) > dpVA2(max_index_A2)) ? max_index_A1 : max_index_A2 + 4;
	vec4 max_A = VA[max_dp_index_A];

	
	const mat4 VBm1 = mat4(VB[0], VB[1], VB[2], VB[3]).transposed();
	const mat4 VBm2 = mat4(VB[4], VB[5], VB[6], VB[7]).transposed();

	const vec4 neg_D = -D;

	vec4 dpVB1 = VBm1 * neg_D;	// now we have the first four dot products in the vector
	vec4 dpVB2 = VBm2 * neg_D;	// and the last four in another

	// find maximum dotp. just brute force for now :D
	int max_index_B1 = 0;
	int max_index_B2 = 0;
	current_max_dp1 = dpVB1(0);
	current_max_dp2 = dpVB2(0);

	for (int i = 1; i < 4; ++i) {
		float dp1 = dpVB1(i);
		if (dp1 > current_max_dp1) {
			max_index_B1 = i;
		}
		float dp2 = dpVB2(i);
		if (dp2 > current_max_dp2) {
			max_index_B2 = i;
		}
	}

	int max_dp_index_B = (dpVB1(max_index_B1) > dpVB2(max_index_B2)) ? max_index_B1 : max_index_B2 + 4;
	vec4 max_B = VB[max_dp_index_B];

	return max_A - max_B;	// minkowski difference, or negative addition
}

typedef bool (*simplexfunc_t)(Simplex*, vec4*);

static bool null_simplexfunc(Simplex *S, vec4 *dir) { return false; }	

static bool line_simplexfunc(Simplex *S, vec4 *dir) {
	vec4 A = S->points[1];
	vec4 AO = -A;	// ie (0,0,0) - A
	vec4 AB = S->points[0] - A;
	if (dot3(AB, AO) > 0) {
		*dir = cross(cross(AB, AO), AB);	// use the edge as next simplex
	}
	else {
		S->clear();
		S->add(A);	// use A as our next simplex
		*dir = AO;
	}
	return true;
}

static bool triangle_simplexfunc(Simplex *S, vec4 *dir) {
		// gets a tad more complicated here :P
		vec4 A = S->points[2];
		vec4 B = S->points[1];
		vec4 C = S->points[0];

		vec4 AO = -A;
		vec4 AB = B-A;
		vec4 AC = C-A;

		vec4 ABC = cross(AB,AC);	// triangle normal
		vec4 ABCxAC = cross(ABC, AC);	// edge normal
		vec4 ABxABC = cross(AB, ABC);

		if (dot3(ABCxAC, AO) > 0) {
			if (dot3(AC, AO) > 0) {
				// use AC as next simplex
				S->clear();
				S->add(A);
				S->add(C);
				*dir = cross(cross(AC, AO), AC);
			}
			else {
				goto resolve;
			}
		}
		else {
			if (dot3(ABxABC, AO) > 0) {
				goto resolve;
			}
			else {
				if (dot3(ABC, AO) > 0) {
					// use triangle as next simplex
					*dir = ABC;
				}
				else {
					// permute these (not 100% sure why though)
					S->points[1] = C;
					S->points[2] = B;
					*dir = -ABC;
				}
			}
		}

		return true;

resolve:
		if (dot3(AB, AO) > 0) {
			S->clear();
			S->add(A);
			S->add(B);
			*dir = cross(cross(AB, AO), AB);
		}
		else {
			S->clear();
			S->add(A);
			*dir = AO;
		}
		return true;

}
static bool tetrahedron_simplexfunc(Simplex *S, vec4 *dir) {
	vec4 A = S->points[3];
	vec4 B = S->points[2];
	vec4 C = S->points[1];
	vec4 D = S->points[0];

	// redundant, but easy to read

	vec4 AB = B-A;
	vec4 AC = C-A;
	vec4 ABxAC = cross(AB, AC);
	if (dot3(ABxAC, A) < 0) { 
		// at least one of these dot products was negative, 
		// so the origin is not enclosed by the tetrahedron.
		// -> use the ABC triangle as our next simplex, and -ABxAC as direction
		S->current_num_points = 3;
		S->points[0] = A;	// after this, p0 = A, p1 = C, p2 = B
		*dir = -ABxAC;
		return false;
	}

	vec4 AD = D-A;
//	vec4 AB = B-A;	// already computed
	vec4 ADxAB = cross(AD, AB);
	if (dot3(ADxAB, A) < 0) {
		S->current_num_points = 3;
		S->points[1] = A;	// p0 = D, p1 = A, p2 = B
		*dir = -ADxAB;
		return false;
	}
	
//	vec4 AC = C-A; // already computed
//	vec4 AD = D-A; // also already computed :D
	vec4 ACxAD = cross(AC,AD);
	if (dot3(ACxAD, A) < 0) {
		S->current_num_points = 3;
		S->points[2] = A; // p0 = D, p1 = C, p2 = A
		*dir = -ACxAD;
		return false;
	}
	
	// we already know that the (inward-facing) BCD-triangle normal will have a negative dot product
	// with OA, so there's no need to recheck that. So if we reached this point, we can conclude that
	// the origin was in fact contained within the tetrahedron, and thus in the Minkowski difference as well,
	// ie. the objects share at least one point => collision.

	return true;

	
}
// jump table :P
static const simplexfunc_t simplexfuncs[5] = { 
	null_simplexfunc,	
	null_simplexfunc,
	line_simplexfunc,
	triangle_simplexfunc,
	tetrahedron_simplexfunc
};



static bool DoSimplex(Simplex *S, vec4 *D) {
	return simplexfuncs[S->current_num_points](S, D);
}

int collision_test_GJK(const OBB &a, const OBB &b) {
	
	vec4 D(1.0, 0.0, 0.0, 0.0);	// initial direction (could be more "educated")

	// compute bounding box vertices.
	
	const float_arr_vec4 a_e(a.e);
	vec4 VA[8];
	vec4 precomp_val_A0 = 2*a_e(0)*a.A0;	// these two recur in the following block, so precompute
	vec4 precomp_val_A2 = 2*a_e(2)*a.A2;


	VA[0] = a.C + a_e(0)*a.A0 + a_e(1)*a.A1 + a_e(2)*a.A2;
	VA[1] = VA[0] - precomp_val_A2;
	VA[2] = VA[1] - precomp_val_A0;
	VA[3] = VA[2] + precomp_val_A2;
	VA[4] = a.C + a_e(0)*a.A0 - a_e(1)*a.A1 + a_e(2)*a.A2;
	VA[5] = VA[4] - precomp_val_A2;
	VA[6] = VA[5] - precomp_val_A0;
	VA[7] = VA[6] + precomp_val_A2;
	
	const float_arr_vec4 b_e(b.e);
	vec4 VB[8];
	vec4 precomp_val_B0 = 2*b_e(0)*b.A0;
	vec4 precomp_val_B2 = 2*b_e(2)*b.A2;

	VB[0] = b.C + b_e(0)*b.A0 + b_e(1)*b.A1 + b_e(2)*b.A2;
	VB[1] = VB[0] - precomp_val_B2;
	VB[2] = VB[1] - precomp_val_B0;
	VB[3] = VB[2] + precomp_val_B2;
	VB[4] = b.C + b_e(0)*b.A0 - b_e(1)*b.A1 + b_e(2)*b.A2;
	VB[5] = VB[4] - precomp_val_B2;
	VB[6] = VB[5] - precomp_val_B0;
	VB[7] = VB[6] + precomp_val_B2;

	// find farthest point (vertex) in the direction D for box a, and in -D for box b.
	vec4 S = GJK_support(D, VA, VB);
	D = -S;

	Simplex simplex;
	simplex.add(S);

	// perhaps add a limit to the number of iterations
	while (1) {
		vec4 A = GJK_support(D, VA, VB);
		if (dot3(A, D) < 0) {
			return 0;
			// we can conclude that there can be no intersection between the two shapes (boxes)
		}
		simplex.add(A);
		if (DoSimplex(&simplex, &D)) { // updates our simplex and the direction
			return 1;
		}
	}

}
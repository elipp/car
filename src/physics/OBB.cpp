#include "physics/OBB.h"

__declspec(align(16)) 
struct float_arr_vec4 {
		__declspec(align(16)) float f[4];
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


static vec4 simplex_points[4];
static int simplex_current_num_points = 0;

static void simplex_add(const vec4 &v) {
	simplex_points[simplex_current_num_points] = v;
	++simplex_current_num_points;
}
static void simplex_clear() {
	simplex_current_num_points = 0;
}

static void simplex_assign(const vec4 &a) {
	simplex_current_num_points = 1;
	simplex_points[0] = a;
}

static void simplex_assign(const vec4 &a, const vec4 &b) {
	simplex_current_num_points = 2;
	simplex_points[0] = b;
	simplex_points[1] = a;
}

static void simplex_assign(const vec4 &a, const vec4 &b, const vec4 &c) {
	simplex_current_num_points = 3;
	simplex_points[0] = c;
	simplex_points[1] = b;
	simplex_points[2] = a;
}

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
	if (R > R0 + R1) { return 1; }

	R0 = a_e(1);
	R1 = dot3(absC_T.column(1), b.e);	
	R = abs_dp_Ai_D(1);
	if (R > R0 + R1) { return 1; }

	R0 = a_e(2);
	R1 = dot3(absC_T.column(2), b.e);
	R = abs_dp_Ai_D(2);
	if (R > R0 + R1) { return 1; }

	// L = Bi
	R0 = dot3(absC.column(0), a.e);
	R1 = b_e(0);
	R = abs_dp_Bi_D(0);
	if (R > R0 + R1) { return 1; }

	R0 = dot3(absC.column(1), a.e);
	R1 = b_e(1);
	R = abs_dp_Bi_D(1);
	if (R > R0 + R1) { return 1; }

	R0 = dot3(absC.column(2), a.e);
	R1 = b_e(2);
	R = abs_dp_Bi_D(2);
	if (R > R0 + R1) { return 1; }

	// various cross products
	
	// L = A0 x B0
	R0 = a_e(1)*absC_T_f(2, 0) + a_e(2)*absC_T_f(1,0);
	R1 = b_e(1)*absC_T_f(0, 2) + b_e(2)*absC_T_f(0,1);
	R = fabs(C_f(0,1)*dp_Ai_D(2) - C_f(0,2)*dp_Ai_D(1));
	if (R > R0 + R1) { return 1; }

	// L = A0 x B1
	R0 = a_e(1)*absC_T_f(2,1) + a_e(2)*absC_T_f(1,1);
	R1 = b_e(1)*absC_T_f(0,2) + b_e(2)*absC_T_f(0,0);
	R = fabs(C_f(1,1)*dp_Ai_D(2) - C_f(1,2)*dp_Ai_D(1));
	if (R > R0 + R1) { return 1; }

	// L = A0 x B2
	R0 = a_e(1)*absC_T_f(2,0) + a_e(2)*absC_T_f(1,0);
	R1 = b_e(1)*absC_T_f(0,1) + b_e(2)*absC_T_f(0,0);
	R = fabs(C_f(2,1)*dp_Ai_D(2) - C_f(2,2)*dp_Ai_D(1));
	if (R > R0 + R1) {}
	
	// L = A1 x B0
	R0 = a_e(0)*absC_T_f(2,0) + a_e(2)*absC_T_f(0,0);
	R1 = b_e(1)*absC_T_f(1,2) + b_e(2)*absC_T_f(1,1);
	R = fabs(C_f(0,2)*dp_Ai_D(0) - C_f(0,0)*dp_Ai_D(2));
	if (R > R0 + R1) { return 1; }

	// L = A1 x B1
	R0 = a_e(0)*absC_T_f(2,1) + a_e(2)*absC_T_f(0,1);
	R1 = b_e(0)*absC_T_f(1,2) + b_e(2)*absC_T_f(1,0);
	R = fabs(C_f(1,2)*dp_Ai_D(0) - C_f(0,0)*dp_Ai_D(2));
	if (R > R0 + R1) { return 1; }

	// L = A1 x B2
	R0 = a_e(0)*absC_T_f(2,2) + a_e(2)*absC_T_f(0,2);
	R1 = b_e(0)*absC_T_f(1,1) + b_e(1)*absC_T_f(1,0);
	R = fabs(C_f(2,2)*dp_Ai_D(0) - C_f(2,0)*dp_Ai_D(2));
	if (R > R0 + R1) { return 1; }
	
	// L = A2 x B0
	R0 = a_e(0)*absC_T_f(1,0) + a_e(1)*absC_T_f(0,0);
	R1 = b_e(1)*absC_T_f(2,2) + b_e(2)*absC_T_f(2,1);
	R = fabs(C_f(0,0)*dp_Ai_D(1) - C_f(0,1)*dp_Ai_D(0));
	if (R > R0 + R1) { return 1; }

	// L = A2 x B1
	R0 = a_e(0)*absC_T_f(1,1) + a_e(1)*absC_T_f(0,1);
	R1 = b_e(0)*absC_T_f(2,2) + b_e(2)*absC_T_f(2,0);
	R = fabs(C_f(1,0)*dp_Ai_D(1) - C_f(1,1)*dp_Ai_D(0));
	if (R > R0 + R1) { return 1; }

	// L = A2 x B2
	R0 = a_e(0)*absC_T_f(1,2) + a_e(1)*absC_T_f(0,2);
	R1 = b_e(0)*absC_T_f(2,1) + b_e(1)*absC_T_f(2,0);
	R = fabs(C_f(2,0)*dp_Ai_D(1) - C_f(2,1)*dp_Ai_D(0));
	if (R > R0 + R1) { return 1; }

	return 0;

}

// GJK (Gilbert-Johnson-Keerthi). References:
// http://mollyrocket.com/849 !!,
// http://www.codezealot.org/archives/88.

static int find_max_dp_index(const float_arr_vec4 &p1, const float_arr_vec4 &p2) {
	
	int max_index_p1 = 0;
	int max_index_p2 = 0;
	float current_max_dp1 = p1(0);
	float current_max_dp2 = p2(0);

	for (int i = 1; i < 4; ++i) {
		float dp1 = p1(i);
		if (dp1 > current_max_dp1) {
			max_index_p1 = i;
			current_max_dp1 = dp1;
		}

		float dp2 = p2(i);
		if (dp2 > current_max_dp2) {
			max_index_p2 = i;
			current_max_dp2 = dp2;
		}
	}
	return p1(max_index_p1) > p2(max_index_p2) ? max_index_p1 : max_index_p2 + 4;
}

vec4 GJKSession::support(const vec4 &D) {
	// returns a point within the Minkowski difference of the two boxes.
	// Basically, we find the vertex that is farthest away in the direction D among
	// the vertices of box A (VA), and the one farthest away in direction -D among
	// the vertices of box B (VB).
	
	float_arr_vec4 dpVA1(VAm1_T*D);	// now we have the first four dot products in the vector
	float_arr_vec4 dpVA2(VAm2_T*D);	// and the last four in another

	// find maximum dotp. just brute force for now :D (there's of course _mm_max_ps, but it's not a very good fit for this
	// since we're not really interested in the dp values themselves)
	int max_dp_A_index = find_max_dp_index(dpVA1, dpVA2);
	vec4 max_A = (max_dp_A_index < 4) ? VAm1.column(max_dp_A_index) : VAm2.column(max_dp_A_index - 4);
	
	const vec4 neg_D = -D;
	float_arr_vec4 dpVB1(VBm1_T*neg_D);	
	float_arr_vec4 dpVB2(VBm2_T*neg_D);	

	int max_dp_B_index = find_max_dp_index(dpVB1, dpVB2);
	vec4 max_B = (max_dp_B_index < 4) ? VBm1.column(max_dp_B_index) : VBm2.column(max_dp_B_index - 4);
	
	return max_A - max_B;	// minkowski difference, or negative addition
}

#define SAMEDIRECTION(va, vb) ((dot3((va), (vb))) > 0)
#define POINTS_TOWARDS_ORIGIN(va) (SAMEDIRECTION(va, AO))

typedef bool (*simplexfunc_t)(vec4*);

// THE PURPOSE OF A "SIMPLEXFUNC" IS TO
// a) FIND THE FEATURE OF THE CURRENT SIMPLEX THAT'S CLOSEST TO THE ORIGIN 
// (EDGE, TRIANGLE FACE), AND SET THAT AS THE NEW SIMPLEX (WITH APPROPRIATE WINDING)
// b) UPDATE SEARCH DIRECTION ACCORDINGLY

static bool null_simplexfunc(vec4 *dir) { return false; }	
static bool line_simplexfunc(vec4 *dir) {
	vec4 AO = -simplex_points[1];		// -A
	vec4 AB = simplex_points[0] + AO;	// essentially B - A
	*dir = cross(cross(AB, AO), AB);	// use this edge as next simplex
	
	// the conclusion of the discussion in https://mollyrocket.com/forums/viewtopic.php?t=271&postdays=0&postorder=asc&start=15
    // was that in the line case, the new point A cannot be the closest simplex feature, so we'll just use the edge AB, and search
	// in a direction that's perpendicular to the edge and towards the origin

	return false;
}

static bool triangle_simplexfunc(vec4 *dir) {
		// gets a tad more complicated here :P
		vec4 A = simplex_points[2];
		vec4 B = simplex_points[1];
		vec4 C = simplex_points[0];

		vec4 AO = -A;
		vec4 AB = B-A;
		vec4 AC = C-A;

		vec4 ABC = cross(AB,AC);	// triangle normal
		vec4 ABCxAC = cross(ABC, AC);	// edge normal, pointing outwards
		vec4 ABxABC = cross(AB, ABC);

		// again, using the same principle as in the line case, the voronoi region of the new point A can be ruled out beforehand

		if (SAMEDIRECTION(ABCxAC, AO)) {
			// then use edge AC
			simplex_assign(A, C);
			*dir = cross(cross(AC, AO), AC);	// a direction perpendicular to the edge, and pointing towards the origin
		}
		else if (SAMEDIRECTION(ABxABC, AO)) {
			// use edge AB
			simplex_assign(A, B);
			*dir = cross(cross(AB, AO), AB);
		}
		else if (SAMEDIRECTION(ABC, AO)) {
			// the origin lies within the triangle area, either "above" or "below"
			*dir = ABC;	
		}
		else {
			// a permutation of the points B & C is needed to give consistent cross products in the tetrahedral simplex processing
			simplex_assign(A, C, B);
			*dir = -ABC;
		}

		return false;

}

static bool tetrahedron_simplexfunc(vec4 *dir) {
	vec4 A = simplex_points[3];
	vec4 B = simplex_points[2];
	vec4 C = simplex_points[1];
	vec4 D = simplex_points[0];

	// redundant, but easy to read
	
	vec4 AO = -A;
	vec4 AB = B-A;
	vec4 AC = C-A;
	vec4 AD = D-A;

	// OUTWARD-FACING (outward from the tetrahedron) triangle normals.
	vec4 ABC = cross(AB, AC);
	vec4 ACD = cross(AC, AD);	
	vec4 ADB = cross(AD, AB);

	// taken from casey's implementation at https://mollyrocket.com/forums/viewtopic.php?t=245&postdays=0&postorder=asc&start=41
	uint32_t code = 0;
	if (POINTS_TOWARDS_ORIGIN(ABC)) {
		code |= 0x01;
	}

	if (POINTS_TOWARDS_ORIGIN(ACD)) {
		code |= 0x02;
	}	

	if (POINTS_TOWARDS_ORIGIN(ADB)) {
		code |= 0x04;
	}

	// the tetrahedron winding tells us there's no need to test for triangle BCD, since
	// that dot product is guaranteed to be negative

	switch(code) {
	case 0:
		// the origin was enclosed by the tetrahedron
		return true;
		break;
	case 1:
		// only in front of triangle ABC
		simplex_assign(A, B, C);
		return triangle_simplexfunc(dir); // because we still need to figure out which feature of the triangle simplex is closest to the origin
		break;
	case 2:
		// only in front of triangle ACD
		simplex_assign(A, C, D);
		return triangle_simplexfunc(dir);
		break;
	case 3: {
		// in front of both ABC and ACD -> differentiate the closest feature of all of the involved points, edges, or faces
		if (POINTS_TOWARDS_ORIGIN(cross(ABC, AC))) {
			if (POINTS_TOWARDS_ORIGIN(cross(AC, ACD))) {
				// there's actually another plane test after this in Casey's implementation, but
				// as said a million times already, the point A can't be closest to the origin
				//if (SAMEDIRECTION(AC, AO)) { // USE [A,C] } else { use [A] } <- unnecessary
				simplex_assign(A, C);
				*dir = cross(cross(AC, AO), AC);
	
			}
			else if (POINTS_TOWARDS_ORIGIN(cross(ACD, AD))) {
				simplex_assign(A, D);	// edge AD
				*dir = cross(cross(AD, AO), AD);
			}
			else {
				simplex_assign(A, C, D);	// the triangle A,C,D.
				*dir = ACD;	
			}
		}
		else if (POINTS_TOWARDS_ORIGIN(cross(AB, ABC))) {
			// again, differentiating between whether the edge AB is closer to the origin than the point A is unnecessary, so just skip
			// if (SAMEDIRECTION(AB, AO)) {
				simplex_assign(A, B);
				*dir = cross(cross(AB, AO), AB);
			//}
			// else { simplex_assign(A); }
		}
		else {
			simplex_assign(A, B, C);
			*dir = ABC;
		}

		break;
		}
	case 4:
		// only in front of ADB
		simplex_assign(A, D, B);
		return triangle_simplexfunc(dir);
		break;
	case 5: {
		// in front of ABC & ADB
		if (POINTS_TOWARDS_ORIGIN(cross(ADB, AB))) {
			if (POINTS_TOWARDS_ORIGIN(cross(AB, ABC))) {
				simplex_assign(A, B);
				*dir = cross(cross(AB, AO), AB);
			}
			else if (POINTS_TOWARDS_ORIGIN(cross(ABC, AC))) {
				simplex_assign(A, C);
				*dir = cross(cross(AC, AO), AC);
			}
			else {
				simplex_assign(A, B, C);
				*dir = ABC;
			}
		}
		else {
			if (POINTS_TOWARDS_ORIGIN(cross(AD, ADB))) {
				// there would be a gratuitous check for SAMEDIRECTION(AD, AO)
				simplex_assign(A, D);
				*dir = cross(cross(AD, AO), AD);
			}
			else {
				simplex_assign(A, D, B);
				*dir = ADB;
			}
		}
		
		break;
	}
	case 6: {
		// in front of ACD & ADB
		if (POINTS_TOWARDS_ORIGIN(cross(ACD, AD))) {
			if (POINTS_TOWARDS_ORIGIN(cross(AD, ADB))) {
				// if SAMEDIRECTION(AD, AO), not needed!
				simplex_assign(A, D);
				*dir = cross(cross(AD, AO), AD);
			}
			else if (POINTS_TOWARDS_ORIGIN(cross(ADB, AB))) {
				simplex_assign(A, B);
				*dir = cross(cross(AB, AO), AB);
			}
			else {
				simplex_assign(A, D, B);
				*dir = ADB;
			}
		}
		else if (POINTS_TOWARDS_ORIGIN(cross(AC, ACD))) {
			// if SAMEDIRECTION(AC, AO), not needed!
			simplex_assign(A, C);
			*dir = cross(cross(AC, AO), AC);
		}
		else {
			simplex_assign(A, C, D);
			*dir = ACD;
		}
		break;
	}
	case 7:
		// in front of all of them (which would imply that the origin is in the voronoi region of the newly added point A -> impossible)
		break;
	default:
		// wtf?
		break;
	}
	return false;
	
}
// jump table :P
static const simplexfunc_t simplexfuncs[5] = { 
	null_simplexfunc,	
	null_simplexfunc,
	line_simplexfunc,
	triangle_simplexfunc,
	tetrahedron_simplexfunc
};



static bool DoSimplex(vec4 *D) {
	std::cerr << "calling simplexfuncs[" << simplex_current_num_points << "] with dir = " << *D << ".\n";
	return simplexfuncs[simplex_current_num_points](D);
}

void OBB::compute_box_vertices(vec4 *vertices_out) const {
	const float_arr_vec4 extents(e);
	vec4 precomp_val_A0 = 2*extents(0)*A0;
	vec4 precomp_val_A1 = 2*extents(1)*A1;
	vec4 precomp_val_A2 = 2*extents(2)*A2;

	vertices_out[0] = C + extents(0)*A0 + extents(1)*A1 + extents(2)*A2;
	vertices_out[1] = vertices_out[0] - precomp_val_A2;
	vertices_out[2] = vertices_out[1] - precomp_val_A0;
	vertices_out[3] = vertices_out[2] + precomp_val_A2;

	vertices_out[4] = vertices_out[0] - precomp_val_A1;
	vertices_out[5] = vertices_out[1] - precomp_val_A1;
	vertices_out[6] = vertices_out[2] - precomp_val_A1;
	vertices_out[7] = vertices_out[3] - precomp_val_A1;
}

GJKSession::GJKSession(const OBB &box_A, const OBB &box_B) {

	vec4 VA[8];
	box_A.compute_box_vertices(VA);

	VAm1 = mat4(VA[0], VA[1], VA[2], VA[3]);
	VAm1_T = VAm1.transposed();	
	VAm2 = mat4(VA[4], VA[5], VA[6], VA[7]);
	VAm2_T = VAm2.transposed();


	vec4 VB[8];
	box_B.compute_box_vertices(VB);

	VBm1 = mat4(VB[0], VB[1], VB[2], VB[3]);
	VBm1_T = VBm1.transposed();
	VBm2 = mat4(VB[4], VB[5], VB[6], VB[7]);
	VBm2_T = VBm2.transposed();

}

int GJKSession::collision_test() {
	
	vec4 D(0.0, 1.0, 0.0, 0.0);	// initial direction (could be more "educated")
	
	// find farthest point (vertex) in the direction D for box a, and in -D for box b (to maximize volume/area for quicker convergence)
	vec4 S = support(D);
	D = -S;

	simplex_assign(S);

	// perhaps add a hard limit to the number of iterations to avoid infinite looping
	while (1) {
		vec4 A = support(D);
		if (dot3(A, D) < 0) {
			// we can conclude that there can be no intersection between the two shapes (boxes)
			return 0;
		}
		simplex_add(A);
		if (DoSimplex(&D)) { // updates our simplex and the search direction in a way that allows us to close in on the origin efficiently
			return 1;
		}
	}

}
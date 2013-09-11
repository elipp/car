#ifndef OBB_H
#define OBB_H

#include "lin_alg.h"
#include <iostream>
#include <sstream>
#include <stdint.h>

enum { GJK_INCONCLUSIVE = -1, GJK_NOCOLLISION = 0, GJK_COLLISION = 1 };
enum { EPA_INCONCLUSIVE = -1, EPA_FAIL = 0, EPA_SUCCESS = 1 };


// TODO: get rid of this!
static inline std::string print_vec4(const vec4 &v) {
	std::ostringstream s;
	s << v;
	return s.str();
}

BEGIN_ALIGN16
struct OBB {
	vec4 A0, A1, A2;	// orthonormal, right-handed axes. 
						// Always updated to reflect the rotation of the host body.
	vec4 e;	// corresponding extents.
	vec4 C;	// center point
	void rotate(Quaternion &q) {
		// this sux, tbh :P
		const mat4 qm = q.toRotationMatrix();
		A0 = (qm*vec4(1.0, 0.0, 0.0, 0.0)).normalized();
		A1 = (qm*vec4(0.0, 1.0, 0.0, 0.0)).normalized();
		A2 = (qm*vec4(0.0, 0.0, 1.0, 0.0)).normalized();
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
} END_ALIGN16;

int collision_test_SAT(const OBB &a, const OBB &b);

BEGIN_ALIGN16
struct mat4_doublet {
	mat4 m[2];
	mat4_doublet(const mat4 &m0, const mat4 &m1) { m[0] = m0; m[1] = m1; }
	mat4_doublet transposed_both() const {
		return mat4_doublet(m[0].transposed(), m[1].transposed());
	}
	mat4 &operator()(int n) { return m[n]; }
	vec4 column(int col) { return col > 3 ? m[1].column(col - 4) : m[0].column(col); }
	mat4_doublet() {};
} END_ALIGN16;

BEGIN_ALIGN16
struct simplex {
	vec4 points[4];
	int current_num_points;

	simplex() { 
		current_num_points = 0;
	}

	void assign(const vec4 &a) {
		current_num_points = 1;
		points[0] = a;
	}

	void assign(const vec4 &a, const vec4 &b) {
		current_num_points = 2;
		points[0] = b;
		points[1] = a;
	}
	void assign(const vec4 &a, const vec4 &b, const vec4 &c) {
		current_num_points = 3;
		points[0] = c;
		points[1] = b;
		points[2] = a;
	}
	void clear() { current_num_points = 0; }

	void add(const vec4 &v) {
		points[current_num_points] = v;
		++current_num_points;
	}
} END_ALIGN16;

BEGIN_ALIGN16
class GJKSession {
private:
	mat4_doublet VAm, VAm_T;
	mat4_doublet VBm, VBm_T;
	vec4 support(const vec4 &D);
	simplex current_simplex;
public:
	// don't like the interface though.
	GJKSession(const OBB &a, const OBB &b);
	int collision_test();
	int EPA_penetration(vec4 *outv);	
	
	simplex get_terminating_simplex() const { return current_simplex; }

	/* these three are for the visualization/debug program */
	int EPA_penetration_stepwise_record(vec4 *outv);

} END_ALIGN16;

// use only for PODs plx
template <typename T, size_t alignment> struct my_aligned_vector {
	
	//void *memblock;
	//T *mem;
#define TEST_CAPACITY 256

	__declspec(align(16)) T mem[TEST_CAPACITY];
	int current_size;
	int capacity;

	int size() const { return current_size; }

	void reallocate(size_t new_size) {
		/*fprintf(stderr, "my_aligned_vector::reallocate: reallocing to a new size of %lld\n", (long long)new_size);
		void *newblock = _aligned_malloc(new_size*sizeof(T), alignment);
		memcpy(newblock, memblock, current_size*sizeof(T));
		_aligned_free(memblock);
		memblock = newblock;
		mem = (T*)newblock;
		capacity = new_size;*/
	}

	my_aligned_vector(size_t capacity_ = TEST_CAPACITY) {
		/*memblock = _aligned_malloc(capacity_*sizeof(T), alignment);
		mem = (T*)memblock;*/
		current_size = 0;
		capacity = capacity_;
	}

	/*~my_aligned_vector() {
//		_aligned_free(memblock);
	}*/

	T* push_back(const T& t) {
		if (current_size >= capacity) {
			reallocate(2*capacity);
		}
		//memcpy(&mem[current_size], &t, sizeof(t));
		mem[current_size] = t;
		++current_size;
		return &mem[current_size - 1];
	}

	void erase(int index) {
		if (index > current_size - 1) { fprintf(stderr, "my_aligned_vector::erase: index (%lld) > current_size - 1 (%lld)!\n", (long long)index, (long long)(current_size - 1)); }
		size_t elements_to_move = current_size - index - 1;
		memmove(&mem[index], &mem[index+1], elements_to_move*sizeof(T));
		--current_size;
	}

	T& operator[](size_t index) {
		return mem[index];
	}

	T* begin() {
		return &mem[0];
	}

	T* end() {
		return &mem[current_size];
	}

	void append(T* from_beg, T* from_end) {

		int num_elements = (from_end - from_beg);
		if (current_size + num_elements >= capacity) {
			size_t diff = (current_size + num_elements) - capacity;
			reallocate(2*current_size);
		}
		size_t blocksize = num_elements*sizeof(T);
		memcpy(&mem[current_size], from_beg, blocksize);
		
		//fprintf(stderr, "my_aligned_vector::append: appending %d elements to position %d\n", (int)num_elements, current_size);
		current_size += num_elements;
		//fprintf(stderr, "current_size after append = %d\n", current_size);

	}

	void clear() {
		current_size = 0;
	}
};

BEGIN_ALIGN16
struct hull_p {
	vec4 p;
	hull_p(const vec4 &v) { p = v; };
	hull_p() {};

	DEFINE_ALIGNED_MALLOC_FREE_MEMBERS;
} END_ALIGN16;

BEGIN_ALIGN16
struct triangle_face {
	vec4 normal;
	hull_p *points[3];
	triangle_face *adjacents[3];	// index 0: edge p0->p1 neighbor, index 1: edge p1->p2 neighbor, index 2: edge p2->p0 neighbor
	float orthog_distance_from_origin;
	int origin_proj_within_triangle;
	int obsolete;

	triangle_face(my_aligned_vector<hull_p, 16> &point_base, int index0, int index1, int index2) {
		points[0] = &point_base[index0];
		points[1] = &point_base[index1];
		points[2] = &point_base[index2];

		adjacents[0] = NULL;
		adjacents[1] = NULL;
		adjacents[2] = NULL;
		
		// the implementation doesn't filter for duplicate point entries, so getting a 0-vector as a normal is indeed possible
		this->normal = cross(points[1]->p - points[0]->p, points[2]->p - points[0]->p);
		
		//if (normal.length3_squared() > 0.001) { normalize }
		// getting a NaN/-1.#IND is actually beneficial to us here; any comparison with a NaN returns false, so 
		// std::sort will sort invalid faces (ie ones with duplicate points -> area of zero) to the bottom of the heap
		this->normal.normalize();	
	
		orthog_distance_from_origin = dot3(this->normal, points[0]->p);	// any of the three points should do.
		origin_proj_within_triangle = contains_origin_proj();
		obsolete = 0;
	}
	triangle_face() {};

	bool is_visible_from(const vec4 &p);	// test whether the face is visible from the point p (a simple plane test)
	bool contains_point(const vec4 &p);
	bool contains_origin_proj();

	DEFINE_ALIGNED_MALLOC_FREE_MEMBERS;
} END_ALIGN16;

struct convex_hull {
	my_aligned_vector<hull_p, 16> points;	// ALL added points (including obsolete ones) are kept in this vector
	my_aligned_vector<triangle_face, 16> faces;	// similarly for faces
	std::vector<triangle_face*> active_faces;
	int add_point(const vec4 &p) { points.push_back(hull_p(p)); return points.size()-1; }
	void purge_triangles_visible_from_point(const vec4 &p);
	int add_face(int index0, int index1, int index2) {
		faces.push_back(triangle_face(points, index0, index1, index2));
		return faces.end() - faces.begin(); 
	}
};


#endif

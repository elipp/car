#ifndef LIN_ALG_H
#define LIN_ALG_H

#ifdef _WIN32
#include <intrin.h>
#include <xmmintrin.h>
#include <smmintrin.h>
#elif __linux__
#include <x86intrin.h>
#include <cpuid.h>
#endif

#include <cstdio>
#include <iostream>
#include <cstring>
#include <cmath>
#include <ostream>

#ifdef _WIN32
#define BEGIN_ALIGN16 __declspec(align(16))
#define END_ALIGN16
#define ALIGNED_MALLOC16(ptr, size) do { ptr = (decltype(ptr))_aligned_malloc((size), 16); } while(0)
#define ALIGNED_FREE(ptr) do { _aligned_free(ptr); } while(0)

#elif __linux__
#define BEGIN_ALIGN16
#define END_ALIGN16 __attribute__((aligned(16)))
#define _ALIGNED_MALLOC16(ptr, size) do { posix_memalign((void**)&(ptr), 16, (size)); } while(0)
#define _ALIGNED_FREE(ptr) do { free(ptr); } while(0)
#endif

#define ALIGNED16(decl) BEGIN_ALIGN16 decl END_ALIGN16

#define DEFINE_ALIGNED_MALLOC_FREE_MEMBERS \
	void *operator new(size_t size)\
	{\
		void *p;\
		ALIGNED_MALLOC16(p, size);\
		return p;\
	}\
	void *operator new[](size_t size) {\
		void *p;\
		ALIGNED_MALLOC16(p, size);\
		return p;\
	}\
		void operator delete(void *p) {\
		ALIGNED_FREE(p);\
	}\
		void operator delete[](void *p) {\
		ALIGNED_FREE(p);\
	}\


class vec4;
class mat4;
class Quaternion;

inline std::ostream &operator<<(std::ostream &out, const __m128 &m) {
	char buffer[128];
	ALIGNED16(float tmp[4]);
	_mm_store_ps(tmp, m);
	sprintf_s(buffer, sizeof(buffer), "(%4.3f, %4.3f, %4.3f, %4.3f)", tmp[0], tmp[1], tmp[2], tmp[3]);
//	sprintf(buffer, "(%4.3f, %4.3f, %4.3f, %4.3f)", tmp[0], tmp[1], tmp[2], tmp[3]);
	out << buffer;
	return out;
}

std::ostream &operator<< (std::ostream& out, const vec4 &v);
std::ostream &operator<< (std::ostream& out, const mat4 &M);
std::ostream &operator<< (std::ostream& out, const Quaternion &q);

#ifndef LINALG_STANDALONE
#include "glwindow_win32.h"
#include "text.h"
#endif

// http://stackoverflow.com/questions/11228855/header-files-for-simd-intrinsics
// (stolen from microsoft :()

#ifndef _MM_TRANSPOSE4_PS
#define _MM_TRANSPOSE4_PS(row0, row1, row2, row3) {                 \
            __m128 tmp3, tmp2, tmp1, tmp0;                          \
                                                                    \
            tmp0   = _mm_shuffle_ps((row0), (row1), 0x44);          \
            tmp2   = _mm_shuffle_ps((row0), (row1), 0xEE);          \
            tmp1   = _mm_shuffle_ps((row2), (row3), 0x44);          \
            tmp3   = _mm_shuffle_ps((row2), (row3), 0xEE);          \
                                                                    \
            (row0) = _mm_shuffle_ps(tmp0, tmp1, 0x88);              \
            (row1) = _mm_shuffle_ps(tmp0, tmp1, 0xDD);              \
            (row2) = _mm_shuffle_ps(tmp2, tmp3, 0x88);              \
            (row3) = _mm_shuffle_ps(tmp2, tmp3, 0xDD);              \
        }
#endif


enum { MAT_ZERO = 0x0, MAT_IDENTITY = 0x1 };

const char* checkCPUCapabilities();

namespace V {
	enum { x = 0, y = 1, z = 2, w = 3 };
}

// microsoft says the __m128 union fields shouldn't be accessed directly as in __m128::m128_f32[n], so.. here we go.
inline void assign_to_field(__m128 &a, int index, float val) {
	ALIGNED16(float tmp[4]);
	_mm_store_ps(tmp, a);
	tmp[index] = val;
	a = _mm_load_ps(tmp);
}

inline float get_field(const __m128 &a, int index) {
	ALIGNED16(float tmp[4]);
	_mm_store_ps(tmp, a);
	return tmp[index];
}

inline float get_first_field(const __m128 &a) {
	return _mm_cvtss_f32(a);
}

__m128 dot3x4_transpose(const mat4 &M, const vec4 &v);
__m128 dot3x4_notranspose(const mat4 &M, const vec4 &v);

__m128 dot4x4_notranspose(const mat4 &M, const vec4 &v);

BEGIN_ALIGN16
class vec4 {		
public:
	__m128 data;

	static const vec4 zero4;
	static const vec4 zero3;

	vec4(const __m128 a) { data = a; }
	// this is necessary for the mat4 class calls
	inline __m128 getData() const { return data; }
	vec4() {};
	vec4(float _x, float _y, float _z, float _w);	
	vec4(const float * const a);

	inline void assign(int col, const float val) {
		assign_to_field(data, col, val);
	}
	inline float operator()(int col) const { 
		return get_field(data, col);
	}
	
	void operator*=(float scalar);
	vec4 operator*(float scalar) const;
	
	void operator/=(float scalar);
	vec4 operator/(float scalar) const;
	
	void operator+=(const vec4& b);
	vec4 operator+(const vec4& b) const;
	
	void operator-=(const vec4& b);
	vec4 operator-(const vec4& b) const;
	vec4 operator-() const; // unary -

	vec4 applyQuatRotation(const Quaternion &q) const;

	float length3() const;
	float length4() const;
	float length3_squared() const;
	float length4_squared() const;

	void normalize();
	vec4 normalized() const;
	void zero();
		
	friend std::ostream &operator<< (std::ostream& out, const vec4 &v);

	void *rawData() const;

	friend float dot3(const vec4 &a, const vec4 &b);
	friend float dot4(const vec4 &a, const vec4 &b);

	friend vec4 abs(const vec4 &a);

	friend vec4 cross(const vec4 &a,  const vec4 &b);
	friend vec4 operator*(float scalar, const vec4& v);

	mat4 toTranslationMatrix() const;
	
	//_DEFINE_ALIGNED_MALLOC_FREE_MEMBERS;

}
END_ALIGN16;

vec4 operator*(float scalar, const vec4& v);	// convenience overload :P


float dot3(const vec4 &a, const vec4 &b);
float dot4(const vec4 &a, const vec4 &b);
vec4 abs(const vec4 &a);

vec4 cross(const vec4 &a,  const vec4 &b);	// not really vec4, since cross product for such vectors isn't defined


BEGIN_ALIGN16 
class mat4 {	// column major 
	
public:
	__m128 data[4];	// each holds a column vector
	static mat4 identity();
	static mat4 zero();
	
	static mat4 proj_ortho(float left, float right, float bottom, float top, float zNear, float zFar);
	static mat4 proj_persp(float left, float right, float bottom, float top, float zNear, float zFar);
	static mat4 proj_persp(float fov_radians, float aspect, float zNear, float zFar); // gluPerspective

	static mat4 rotate(float angle_radians, float axis_x, float axis_y, float axis_z);
	static mat4 scale(float x, float y, float z);
	static mat4 translate(float x, float y, float z);
	static mat4 translate(const vec4 &v);

	mat4() {};
	mat4(const float *data);
	mat4(const float main_diagonal_val);
	mat4(const vec4& c1, const vec4& c2, const vec4& c3, const vec4& c4);
	mat4(const __m128& c1, const __m128& c2, const __m128& c3, const __m128& c4);

	mat4 &operator=(const mat4 &M) { memcpy(&this->data[0], &M.data[0], 4*sizeof(__m128)); return *this; }

	inline void assign(int col, int row, float val) { assign_to_field(data[col], row, val); }
	inline float operator()(int col, int row) const { return get_field(data[col], row); }
	
	mat4 operator* (const mat4 &R) const;
	vec4 operator* (const vec4 &R) const;

	void operator*=(const mat4 &R);	// performs _right-side_ multiplication, ie. (*this) = (*this) * R
	
	vec4 row(int i) const;
	vec4 column(int i) const;

	void assignToRow(int row, const vec4& v);
	void assignToColumn(int column, const vec4& v);
	
	void transpose();
	mat4 transposed() const;

	void invert();
	mat4 inverted() const;
	
	void *rawData() const;	// returns a column-major float[16]
		
	friend mat4 abs(const mat4 &m); // perform fabs on all elements of argument matrix
	friend mat4 operator*(float scalar, const mat4& m);
	friend float det(const mat4 &m);

	//_DEFINE_ALIGNED_MALLOC_FREE_MEMBERS;

} END_ALIGN16;

mat4 abs(const mat4 &m);
mat4 operator*(float scalar, const mat4& m);

float det(const mat4 &m);

// the quaternion memory layout is as presented in the following enum

namespace Q {
	enum {x = 0, y = 1, z = 2, w = 3};
};

BEGIN_ALIGN16 
class Quaternion {
public:
	__m128 data;

	Quaternion(float x, float y, float z, float w);
	Quaternion(const __m128 d) { data = d;};
	Quaternion();
	
	inline void assign(int col, float val) { assign_to_field(data, col, val); }
	inline float operator()(int col) const { return get_field(data, col); }
	
	inline __m128 getData() const { return data; }

	void normalize();
	Quaternion conjugate() const;

	friend std::ostream &operator<< (std::ostream& out, const Quaternion &q);
	
	void operator*=(const Quaternion &b);
	Quaternion operator*(const Quaternion& b) const;

	void operator*=(float scalar);
	Quaternion operator*(float scalar) const;

	void operator+=(const Quaternion &b);
	Quaternion operator+(const Quaternion& b) const;

	//vec4 operator*(const vec4& b) const;
	
	static Quaternion fromAxisAngle(float x, float y, float z, float angle_radians);
	mat4 toRotationMatrix() const;

	friend Quaternion operator*(float scalar, const Quaternion &q);

	//_DEFINE_ALIGNED_MALLOC_FREE_MEMBERS;

} END_ALIGN16;

Quaternion operator*(float scalar, const Quaternion &q);

// these are pretty useful for when there's need to manipulate many of the data fields in a __m128

BEGIN_ALIGN16
struct float_arr_vec4 {
	ALIGNED16(float f[4]);
	inline void operator=(const vec4 &v) {
		_mm_store_ps(f, v.getData());
	}
	float_arr_vec4(const vec4 &v) {
		_mm_store_ps(f, v.getData());
	}
	inline float operator()(int col) const { return f[col]; }
	vec4 as_vec4() const { return vec4(_mm_load_ps(f)); }
} END_ALIGN16;

BEGIN_ALIGN16 
struct float_arr_mat4 {
	ALIGNED16(float f[16]);
	inline void operator=(const mat4 &m) {
		memcpy(f, m.rawData(), 16*sizeof(float));
	}
	float_arr_mat4(const mat4 &m) {
		memcpy(f, m.rawData(), 16*sizeof(float));
	}

	inline float& operator()(int col, int row) { return f[4*col + row]; }
	mat4 as_mat4() const { 
		return mat4(f);
	}
} END_ALIGN16;


#endif

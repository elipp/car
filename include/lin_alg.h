#ifndef LIN_ALG_H
#define LIN_ALG_H

#ifdef _WIN32
#include <intrin.h>
#include <xmmintrin.h>
#include <smmintrin.h>
#elif __linux__
#include <x86intrin.h>
#endif

#include <cstdio>
#include <cstring>
#include <cmath>
#include <ostream>

class vec4;
class mat4;
class Quaternion;

std::ostream &operator<< (std::ostream& out, const vec4 &v);
std::ostream &operator<< (std::ostream& out, const mat4 &M);
std::ostream &operator<< (std::ostream& out, const Quaternion &q);

#include "glwindow_win32.h"
#include "text.h"


// http://stackoverflow.com/questions/11228855/header-files-for-simd-intrinsics

// stolen from microsoft :(

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

typedef struct _cpuid_t {
	int eax, ebx, ecx, edx;
} cpuid_t;

const char* checkCPUCapabilities();

// forward declarations 

namespace V {
	enum { x = 0, y = 1, z = 2, w = 3 };
}

#ifdef _WIN32
__declspec(align(16)) // to ensure 16-byte alignment in memory
#endif
class vec4 {		
	__m128 data;
	static const vec4 zero_const;
public:
	vec4(const __m128 a) { data = a; }
	// this is necessary for the mat4 class calls
	inline __m128 getData() const { return data; }
	vec4();	
	vec4(float _x, float _y, float _z, float _w);	
	vec4(const float * const a);

	inline float& operator() (int row) { return data.m128_f32[row]; }
	inline float element(int row) const { return data.m128_f32[row]; }

	// see also: vec4 operator*(const float& scalar, vec4& v);

	void operator*=(float scalar);
	vec4 operator*(float scalar) const;
	
	void operator/=(float scalar);
	vec4 operator/(float scalar) const;
	
	void operator+=(const vec4& b);
	vec4 operator+(const vec4& b) const;
	
	void operator-=(const vec4& b);
	vec4 operator-(const vec4& b) const;
	vec4 operator-() const; // unary -

	float length3() const;
	float length4() const;

	void normalize();
	vec4 normalized() const;
	void zero();
	
	friend std::ostream &operator<< (std::ostream& out, const vec4 &v);

	void *rawData() const;

	friend float dot(const vec4 &a, const vec4 &b);
	friend vec4 cross(const vec4 &a,  const vec4 &b);
	friend vec4 operator*(float scalar, const vec4& v);

	mat4 toTranslationMatrix() const;

	void *operator new(size_t size) {
		void *p;
#ifdef _WIN32
		p = _aligned_malloc(size, 16);
#elif __linux__
		posix_memalign(&p, 16, size);
#endif
		return p;
	}
	void *operator new[](size_t size) {
		void *p;
#ifdef _WIN32
		p = _aligned_malloc(size, 16);
#elif __linux__
		posix_memalign(&p, 16, size);
#endif
		return p;
	}

	void operator delete(void *p) {
#ifdef _WIN32
		_aligned_free(p);
#elif __linux__
		vec4 *v = static_cast<vec4*>(p);	// the cast could be unnecessary
		free(v);
#endif
	}

	void operator delete[](void *p) {
#ifdef _WIN32
		_aligned_free(p);
#elif __linux__
		vec4 *v = static_cast<vec4*>(p);	// the cast could be unnecessary
		free(v);
#endif	// the cast could be unnecessary
	}


} 
#ifdef __linux__
__attribute__((aligned(16))) // to ensure 16-byte alignment in memory
#endif
;

vec4 operator*(float scalar, const vec4& v);	// convenience overload :P
// no operator/ for <scalar>/<vec4>, since the operation doesn't exist


// NOTE: the dot function doesn't perform an actual dot product computation of two R^4 vectors,
// as the type of the arguments misleadingly suggests. Instead it computes a truncated dot product,
// including only the first 3 components (i.e. x,y,z).

float dot(const vec4 &a, const vec4 &b);
// dot benchmarks for 100000000 iterations:
// naive:			20.9s
// DPPS:			2.9s
// MULPS:			3.0s

vec4 cross(const vec4 &a,  const vec4 &b);	// not really vec4, since cross product for such vectors isn't defined


#ifdef _WIN32
__declspec(align(16)) // to ensure 16-byte alignment in memory
#endif
class mat4 {	// column major 

	__m128 data[4];	// each holds a column vector
	
public:
	static mat4 identity();
	static mat4 zero();
	
	static mat4 proj_ortho(float left, float right, float bottom, float top, float zNear, float zFar);
	static mat4 proj_persp(float left, float right, float bottom, float top, float zNear, float zFar);
	static mat4 proj_persp(float fov_radians, float aspect, float zNear, float zFar); // gluPerspective-like

	static mat4 rotate(float angle_radians, float axis_x, float axis_y, float axis_z);
	static mat4 scale(float x, float y, float z);
	static mat4 translate(float x, float y, float z);
	static mat4 translate(const vec4 &v);

	inline float& operator()(int column, int row) { return data[column].m128_f32[row]; }
	inline float elementAt(int column, int row) const { return data[column].m128_f32[row]; }
	
	mat4 operator* (const mat4 &R) const;
	vec4 operator* (const vec4 &R) const;

	void operator*=(const mat4 &R);	// performs _right-side_ multiplication, ie. (*this) = (*this) * R
	
	vec4 row(int i) const;
	vec4 column(int i) const;

	void assignToRow(int row, const vec4& v);
	void assignToColumn(int column, const vec4& v);
	
	mat4();
	mat4(const float *data);
	mat4(const int main_diagonal_val);
	mat4(const vec4& c1, const vec4& c2, const vec4& c3, const vec4& c4);
	mat4(const __m128& c1, const __m128& c2, const __m128& c3, const __m128& c4);

	void transpose();
	mat4 transposed() const;

	void invert();
	mat4 inverted() const;
	
	void *rawData() const;	// returns a column-major float[16]
	void printRaw() const;	// prints elements in actual memory order.
		


	friend mat4 operator*(float scalar, const mat4& m);
	
	void *operator new(size_t size) {
		void *p;
#ifdef _WIN32
		p = _aligned_malloc(size, 16);
#elif __linux__
		posix_memalign(&p, 16, size);
#endif
		return p;
	}
	void *operator new[](size_t size) {
		void *p;
#ifdef _WIN32
		p = _aligned_malloc(size, 16);
#elif __linux__
		posix_memalign(&p, 16, size);
#endif
		return p;
	}

	void operator delete(void *p) {
#ifdef _WIN32
		_aligned_free(p);
#elif __linux__
		mat4 *m = static_cast<mat4*>(p);
		free(m);
#endif
	}

	void operator delete[](void *p) {
#ifdef _WIN32
		_aligned_free(p);
#elif __linux__
		mat4 *m = static_cast<mat4*>(p);
		free(m);
#endif
	}
}
#ifdef __linux__
__attribute__((aligned(16))) // to ensure 16-byte alignment in memory
#endif
;

mat4 operator*(float scalar, const mat4& m);

inline float det(const mat4 &m);

// the quaternion memory layout is as presented in the following enum

namespace Q {
	enum {x = 0, y = 1, z = 2, w = 3};
};

#ifdef _WIN32
__declspec(align(16)) // to ensure 16-byte alignment in memory
#endif
class Quaternion {
	__m128 data;
public:

	Quaternion(float x, float y, float z, float w);
	Quaternion(const __m128 d) { data = d;};
	Quaternion();
	
	inline float element(int i) const { return data.m128_f32[i]; }
	inline float& operator()(int i) { return data.m128_f32[i]; }
	
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

	vec4 operator*(const vec4& b) const;
	
	static Quaternion fromAxisAngle(float x, float y, float z, float angle);
	mat4 toRotationMatrix() const;

	friend Quaternion operator*(float scalar, const Quaternion &q);

	void *operator new(size_t size) {
		void *p;
#ifdef _WIN32
		p = _aligned_malloc(size, 16);
#elif __linux__
		posix_memalign(&p, 16, size);
#endif
		return p;
	}
	void *operator new[](size_t size) {
		void *p;
#ifdef _WIN32
		p = _aligned_malloc(size, 16);
#elif __linux__
		posix_memalign(&p, 16, size);
#endif
		return p;
	}

	void operator delete(void *p) {
#ifdef _WIN32
		_aligned_free(p);
#elif __linux__
		Quaternion *q = static_cast<Quaternion*>(p);	// the cast could be unnecessary
		free(q);
#endif
	}

	void operator delete[](void *p) {
#ifdef _WIN32
		_aligned_free(p);
#elif __linux__
		Quaternion *q = static_cast<Quaternion*>(p);	// the cast could be unnecessary
		free(q);
#endif
	}
} 
#ifdef __linux__
__attribute__((aligned(16)))
#endif
;

Quaternion operator*(float scalar, const Quaternion &q);

#endif
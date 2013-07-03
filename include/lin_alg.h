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
#include <iostream>
#include <cstring>
#include <cmath>
#include <ostream>

class vec4;
class mat4;
class Quaternion;

inline std::ostream &operator<<(std::ostream &out, const __m128 &m) {
	char buffer[128];
	__declspec(align(16)) float tmp[4];
	_mm_store_ps(tmp, m);
	sprintf_s(buffer, 128, "(%4.3f, %4.3f, %4.3f, %4.3f)", tmp[0], tmp[1], tmp[2], tmp[3]);
	out << buffer;
	return out;
}

std::ostream &operator<< (std::ostream& out, const vec4 &v);
std::ostream &operator<< (std::ostream& out, const mat4 &M);
std::ostream &operator<< (std::ostream& out, const Quaternion &q);

#include "glwindow_win32.h"
#include "text.h"

float MM_DPPS_XYZ(__m128 a, __m128 b);
float MM_DPPS_XYZW(__m128 a, __m128 b);

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

const char* checkCPUCapabilities();

// forward declarations 

namespace V {
	enum { x = 0, y = 1, z = 2, w = 3 };
}

// microsoft says the __m128 union fields shouldn't be accessed directly, so.. here we go ^^_^^.
inline void assign_to_field(__m128 &a, int index, float val) {
	__declspec(align(16)) float tmp[4];
	_mm_store_ps(tmp, a);
	tmp[index] = val;
	a = _mm_load_ps(tmp);
}

inline float get_field(const __m128 &a, int index) {
	__declspec(align(16)) float tmp[4];
	_mm_store_ps(tmp, a);
	return tmp[index];
}

inline float get_first_field(const __m128 &a) {
	float r;
	_mm_store_ss(&r, a);
	//std::cerr << "get_first_field: returning " << r << "\n";
	return r;
}

#ifdef _WIN32
__declspec(align(16)) // to ensure 16-byte alignment in memory
#endif
class vec4 {		
	__m128 data;
public:
	
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

	vec4 applyQuatRotation(const Quaternion &q) const;

	float length3() const;
	float length4() const;

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


float dot3(const vec4 &a, const vec4 &b);
float dot4(const vec4 &a, const vec4 &b);
vec4 abs(const vec4 &a);

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

	inline void assign(int col, int row, float val) { assign_to_field(data[col], row, val); }
	inline float operator()(int col, int row) const { return get_field(data[col], row); }
	
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
		

	friend mat4 abs(const mat4 &m); // perform fabs on all elements of argument matrix
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

mat4 abs(const mat4 &m);
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

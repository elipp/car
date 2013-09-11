#include "lin_alg.h"

static const __m128 ZERO = _mm_setzero_ps();
static const __m128 MINUS_ONES = _mm_set_ps(1.0, -1.0, -1.0, -1.0);
static const __m128 QUAT_CONJUGATE = _mm_set_ps(1.0, -1.0, -1.0, -1.0);	// in reverse order!
static const __m128 QUAT_NO_ROTATION = _mm_set_ps(1.0, 0.0, 0.0, 0.0);
static const int mask3021 = 0xC9, // 11 00 10 01_2
				mask3102 = 0xD2, // 11 01 00 10_2
				mask1032 = 0x4E, // 01 00 11 10_2
				xyz_dot_mask = 0x71, // 01 11 00 01_2
				xyzw_dot_mask = 0xF1; // 11 11 00 01_2


static const float identity_arr[16] = { 1.0, 0.0, 0.0, 0.0,
					0.0, 1.0, 0.0, 0.0, 
					0.0, 0.0, 1.0, 0.0, 
					0.0, 0.0, 0.0, 1.0 };

static const float zero_arr[16] = { 0.0, 0.0, 0.0, 0.0,
				    0.0, 0.0, 0.0, 0.0,
				    0.0, 0.0, 0.0, 0.0,
				    0.0, 0.0, 0.0, 0.0 };
								

const vec4 vec4::zero4 = vec4(ZERO);
const vec4 vec4::zero3 = vec4(0.0, 0.0, 0.0, 1.0);
const mat4 identity_const_mat4 = mat4(identity_arr);
const mat4 zero_const_mat4 = mat4(zero_arr);

__m128 dot3x4_transpose(const mat4 &M, const vec4 &v) {

	__m128 tmp3, tmp2, tmp1, tmp0;                          
	__m128 row2, row1, row0;

	tmp0   = _mm_shuffle_ps((M.data[0]), (M.data[1]), 0x44);          
	tmp2   = _mm_shuffle_ps((M.data[0]), (M.data[1]), 0xEE);          
	tmp1   = _mm_shuffle_ps((M.data[2]), (M.data[3]), 0x44);          
	tmp3   = _mm_shuffle_ps((M.data[2]), (M.data[3]), 0xEE);          

	row0 = _mm_shuffle_ps(tmp0, tmp1, 0x88);              
	row1 = _mm_shuffle_ps(tmp0, tmp1, 0xDD);              
	row2 = _mm_shuffle_ps(tmp2, tmp3, 0x88);              

	const __m128 Vx = _mm_shuffle_ps(v.data, v.data, 0x00); 
	const __m128 Vy = _mm_shuffle_ps(v.data, v.data, 0x55);
	const __m128 Vz = _mm_shuffle_ps(v.data, v.data, 0xAA); 

	__m128 xx = _mm_mul_ps(Vx, row0);
	__m128 yy = _mm_mul_ps(Vy, row1);
	__m128 zz = _mm_mul_ps(Vz, row2);

	__m128 ret = _mm_add_ps(xx, yy);
	ret = _mm_add_ps(ret, zz);

	return ret;

}

__m128 dot3x4_notranspose(const mat4 &M, const vec4 &v) {

	const __m128 Vx = _mm_shuffle_ps(v.data, v.data, 0x00); // (Vx, Vx, Vx, Vx)
	const __m128 Vy = _mm_shuffle_ps(v.data, v.data, 0x55); // (Vy, Vy, Vy, Vy)
	const __m128 Vz = _mm_shuffle_ps(v.data, v.data, 0xAA); // (Vz, Vz, Vz, Vz)

	__m128 xx = _mm_mul_ps(Vx, M.data[0]);
	__m128 yy = _mm_mul_ps(Vy, M.data[1]);
	__m128 zz = _mm_mul_ps(Vz, M.data[2]);

	__m128 ret = _mm_add_ps(xx, yy);
	ret = _mm_add_ps(ret, zz);

	return ret;

}

__m128 dot4x4_notranspose(const mat4 &M, const vec4 &v) {
	const __m128 Vx = _mm_shuffle_ps(v.data, v.data, 0x00); // (Vx, Vx, Vx, Vx)
	const __m128 Vy = _mm_shuffle_ps(v.data, v.data, 0x55); // (Vy, Vy, Vy, Vy)
	const __m128 Vz = _mm_shuffle_ps(v.data, v.data, 0xAA); // (Vz, Vz, Vz, Vz)
	const __m128 Vw = _mm_shuffle_ps(v.data, v.data, 0xFF); // (Vw, Vw, Vw, VW)
	
	__m128 xx = _mm_mul_ps(Vx, M.data[0]);
	__m128 yy = _mm_mul_ps(Vy, M.data[1]);
	__m128 zz = _mm_mul_ps(Vz, M.data[2]);
	__m128 ww = _mm_mul_ps(Vw, M.data[3]);

	__m128 ret = _mm_add_ps(xx, yy);
	ret = _mm_add_ps(ret, zz);
	ret = _mm_add_ps(ret, ww);
	return ret;
}

static const __m128i _and_mask = _mm_set_epi32(0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
static const __m128 and_mask_0111 = *((__m128*)&_and_mask);

inline float MM_DPPS_XYZ_SSE(__m128 a, __m128 b) {
	const __m128 mul = _mm_and_ps(_mm_mul_ps(a, b), and_mask_0111);
	const __m128 t = _mm_add_ps(mul, _mm_movehl_ps(mul, mul));
	const __m128 sum = _mm_add_ss(t, _mm_shuffle_ps(t, t, 1));
	return get_first_field(sum);
}

inline float MM_DPPS_XYZW_SSE(__m128 a, __m128 b) {
	const __m128 mul = _mm_mul_ps(a, b);
	const __m128 t = _mm_add_ps(mul, _mm_movehl_ps(mul, mul));
	const __m128 sum = _mm_add_ss(t, _mm_shuffle_ps(t, t, 1));
	return get_first_field(sum);

}

#ifdef __SSE3__	// these aren't defined on windows tho
inline float MM_DPPS_XYZ_SSE3(__m128 a, __m128 b) {
	const __m128 mul = _mm_and_ps(_mm_mul_ps(a, b), and_mask_0111);
	 __m128 t = _mm_hadd_ps(mul, mul);	// t = (ax*bx + ay*by, az*bz + 0, ax*bx + ay*by, az*bz + 0)
	 t = _mm_hadd_ps(t, t);				// t = (ax*bx + ay*by + az*bz + 0, ax*bx + ay*by + az*bz + 0, sim., sim.)
	 return get_first_field(t);
}

inline float MM_DPPS_XYZW_SSE3(__m128 a, __m128 b) {
	const __m128 mul = _mm_mul_ps(a, b);
	__m128 t = _mm_hadd_ps(mul, mul);
	t = _mm_hadd_ps(t, t);
	return get_first_field(t);
}
#endif

#ifdef __SSE4_1__
inline float MM_DPPS_XYZ_SSE41(__m128 a, __m128 b) {
	return get_first_field(_mm_dp_ps(a, b, xyz_dot_mask));
}

inline float MM_DPPS_XYZW_SSE41(__m128 a, __m128 b) {
	return get_first_field(_mm_dp_ps(a, b, xyzw_dot_mask));
}
#endif

#define MM_DPPS_XYZ(a, b) (MM_DPPS_XYZ_SSE((a),(b)))
#define MM_DPPS_XYZW(a, b) (MM_DPPS_XYZW_SSE((a),(b)))

const char* checkCPUCapabilities() {

	typedef struct _cpuid_t {
		unsigned int eax, ebx, ecx, edx;
	} cpuid_t;

	cpuid_t c;
	memset(&c, 0, sizeof(c));
#ifdef _WIN32
	__cpuid((int*)&c, 1);
#elif __linux__
	__get_cpuid(1, &c.eax, &c.ebx, &c.ecx, &c.edx);
#endif

	// would be better to somehow check these flags at compile time

#define SSE_BIT_ENABLED ((c.edx & 0x02000000) == 0x02000000) // register edx, bit 25
#define SSE3_BIT_ENABLED ((c.ecx & 0x00000001) == 0x00000001) // register ecx, bit 0
#define SSE41_BIT_ENABLED ((c.ecx & 0x00080000) == 0x00080000) // register ecx, bit 19
	
	if (!SSE_BIT_ENABLED) {
		return "ERROR: SSE not supported by host processor!";
	}
	
	return "Using SSE1 for dot product computation.\n";
}

vec4::vec4(float _x, float _y, float _z, float _w) {
	// must use _mm_setr_ps for this constructor. Note 'r' for reversed.
	//data = _mm_setr_ps(_x, _y, _z, _w);
	data = _mm_set_ps(_w, _z, _y, _x);	// could be faster, haven't tested

}


vec4::vec4(const float* const a) {
	data = _mm_load_ps(a);		// not assuming 16-byte alignment for a.
}

void vec4::operator*=(float scalar) {
	data = _mm_mul_ps(data, _mm_set1_ps(scalar));
}

vec4 operator*(float scalar, const vec4& v) {
	vec4 r(v);
	r *= scalar;
	return r;
}	

vec4 vec4::operator*(float scalar) const{
	vec4 v(*this);
	v *= scalar;
	return v;
}

void vec4::operator/=(float scalar) {
	const __m128 scalar_recip = _mm_set1_ps(1.0/scalar);
	this->data = _mm_mul_ps(this->data, scalar_recip);
}

vec4 vec4::operator/(float scalar) const {
	vec4 v = (*this);
	v /= scalar;
	return v;
}

void vec4::operator+=(const vec4 &b) {
	this->data=_mm_add_ps(data, b.data);
}

vec4 vec4::operator+(const vec4 &b) const {
	vec4 v = (*this);
	v += b; 
	return v;
}

void vec4::operator-=(const vec4 &b) {
	this->data=_mm_sub_ps(data, b.data);
}

vec4 vec4::operator-(const vec4 &b) const {
	vec4 v = (*this);
	v -= b;
	return v;
}

vec4 vec4::operator-() const { 
	return vec4(_mm_mul_ps(MINUS_ONES, this->data));
}


vec4 vec4::applyQuatRotation(const Quaternion &q) const {
	
	vec4 v(*this);
 	Quaternion vec_q(this->getData()), res_q;
	vec_q.assign(Q::w, 0);

	res_q = vec_q * q.conjugate();
	res_q = q*res_q;

	vec4 r(res_q.getData());
	r.assign(V::w, 1.0);
	
	return r;
}

float vec4::length3() const {
	//return sqrt(_mm_dp_ps(this->data, this->data, xyz_dot_mask).m128_f32[0]);
	return sqrt(MM_DPPS_XYZ(this->data, this->data));
}

float vec4::length4() const {
	return sqrt(MM_DPPS_XYZW(this->data, this->data));
}	

float vec4::length3_squared() const {
	return MM_DPPS_XYZ(this->data, this->data);
}

float vec4::length4_squared() const {
	return MM_DPPS_XYZW(this->data, this->data);
}

// this should actually include all components, but given the application, this won't :P
void vec4::normalize() {
	const __m128 factor = _mm_rsqrt_ps(_mm_set1_ps(MM_DPPS_XYZ(this->data, this->data)));	// rsqrtps = approximation :P there will be issues if dot3 == 0!
	this->data = _mm_mul_ps(factor, this->data);
}

vec4 vec4::normalized() const {
	vec4 v(*this);
	v.normalize();
	return v;
}

void vec4::zero() {
	(*this) = vec4::zero4;
}

std::ostream &operator<< (std::ostream& out, const vec4 &v) {
	out << v.getData();
	return out;
}

void* vec4::rawData() const {
	return (void*)&data;
}


float dot3(const vec4 &a, const vec4 &b) {
	return MM_DPPS_XYZ(a.data, b.data);
}

float dot4(const vec4 &a, const vec4 &b) {
	return MM_DPPS_XYZW(a.data, b.data);
}

vec4 abs(const vec4 &a) {
	static const __m128 mask = _mm_set1_ps(-0.f);	// 1 << 31
	return vec4(_mm_andnot_ps(mask, a.getData()));
}

vec4 cross(const vec4 &a, const vec4 &b) {

	// "vec4 cross" in quotes, since the cross product only really exists for vec3 (and vec7 I think :D)
	// See: http://fastcpp.blogspot.fi/2011/04/vector-cross-product-using-sse-code.html.
	// Absolutely beautiful! (the exact same recipe can be found at
	// http://neilkemp.us/src/sse_tutorial/sse_tutorial.html#E, albeit in assembly.)
	
		
	return vec4(
	_mm_sub_ps(
    _mm_mul_ps(_mm_shuffle_ps(a.data, a.data, mask3021), _mm_shuffle_ps(b.data, b.data, mask3102)),
    _mm_mul_ps(_mm_shuffle_ps(a.data, a.data, mask3102), _mm_shuffle_ps(b.data, b.data, mask3021))
  )
  );

}

mat4 vec4::toTranslationMatrix() const {
	mat4 M = mat4::identity();
	M.assignToColumn(3, (*this));
	return M;
}

mat4::mat4(const float *const arr) {
	// this is assuming arr is aligned to a 16-byte boundary, and column major
	memcpy(&data[0], arr, 16*sizeof(float));
}

mat4::mat4(float main_diagonal_val) {
	const __m128 d = _mm_set_ss(main_diagonal_val);
	this->data[0] = d;
	this->data[1] = _mm_shuffle_ps(d, d, _MM_SHUFFLE(1, 1, 0, 1));
	this->data[2] = _mm_shuffle_ps(d, d, _MM_SHUFFLE(1, 0, 1, 1));
	this->data[3] = _mm_shuffle_ps(d, d, _MM_SHUFFLE(0, 1, 1, 1));

}


mat4::mat4(const vec4& c1, const vec4& c2, const vec4& c3, const vec4& c4) {
	data[0] = c1.getData();
	data[1] = c2.getData();
	data[2] = c3.getData();
	data[3] = c4.getData();
}

// must be passed as references, since otherwise the fourth one will lose its alignment
mat4::mat4(const __m128& c1, const __m128&  c2, const __m128&  c3, const __m128& c4) {
	data[0] = c1;
	data[1] = c2;
	data[2] = c3;
	data[3] = c4;
}

mat4 mat4::rotate(float angle_radians, float axis_x, float axis_y, float axis_z) {
	Quaternion q = Quaternion::fromAxisAngle(axis_x, axis_y, axis_z, angle_radians);
	return q.toRotationMatrix();
}

mat4 mat4::operator* (const mat4& R) const {
	
	// we'll choose to transpose the other matrix, and instead of calling the perhaps
	// more intuitive row(), we'll be calling column(), which is a LOT faster in comparison.
	/*
	mat4 L = (*this).transposed();	
	mat4 ret;
#pragma loop(hint_parallel(4))
	for (int i = 0; i < 4; i++) {
		ALIGNED16(float tmp[4]);	// represents a temporary column
		for (int j = 0; j < 4; j++) {
			tmp[j] = MM_DPPS_XYZW(L.data[j], R.data[i]);
		}
		ret.assignToColumn(i, _mm_load_ps(tmp));
	}
	return ret; */


	
	mat4 L = (*this);
	mat4 ret;
	
	ret.data[0] = dot4x4_notranspose(L, R.data[0]);
	ret.data[1] = dot4x4_notranspose(L, R.data[1]);
	ret.data[2] = dot4x4_notranspose(L, R.data[2]);
	ret.data[3] = dot4x4_notranspose(L, R.data[3]);

	return ret; 

}


vec4 mat4::operator* (const vec4& R) const {

	// try with temporary mat4? :P
	// result: performs better (with optimizations disabled at least)
	//const mat4 M = (*this).transposed();
	const mat4 M = (*this);
/*	ALIGNED16(float tmp[4]);	// represents a temporary column

	for (int i = 0; i < 4; i++) {
		tmp[i] = MM_DPPS_XYZW(M.data[i], R.getData());
	}*/

	return vec4(dot4x4_notranspose(M, R));
}

void mat4::operator*=(const mat4 &R) {
	(*this) = (*this) * R;
}

mat4 operator*(float scalar, const mat4& m) {
	const __m128 scalar_m128 = _mm_set1_ps(scalar);
	return mat4(_mm_mul_ps(scalar_m128, m.data[0]),
				_mm_mul_ps(scalar_m128, m.data[1]),
				_mm_mul_ps(scalar_m128, m.data[2]),
				_mm_mul_ps(scalar_m128, m.data[3]));

}

mat4 mat4::identity() {
	// better design than to do (*this)(0,0) = 1.0 etc
	return identity_const_mat4;
}
mat4 mat4::zero() {
	return zero_const_mat4;
}


vec4 mat4::row(int i) const {
	return this->transposed().data[i];
}

vec4 mat4::column(int i) const {
	return this->data[i];
}

void mat4::assignToColumn(int column, const vec4& v) {
	this->data[column] = v.getData();
}

void mat4::assignToRow(int row, const vec4& v) {
	this->transpose();
	this->data[row] = v.getData();
	this->transpose();
}

void *mat4::rawData() const {
	return (void*)&data[0];
}

std::ostream &operator<< (std::ostream& out, const mat4 &M) {
	mat4 T = M.transposed();
	// the mat4 class operates on a column-major basis, so in order to see
	// the matrix data output quickly enough and in a familiar format, we need to 
	// transpose it first. This occurs throughout, because row() is much, much slower :P
	out << T.column(0).getData() << "\n"
		<< T.column(1).getData() << "\n"
		<< T.column(2).getData() << "\n"
		<< T.column(3).getData();
	return out;
}

mat4 mat4::proj_ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
		
	float_arr_mat4 M(mat4::identity());
	
	M(0,0) = 2.0/(right - left);

	M(1,1) = 2.0/(top - bottom);
	M(2,2) = -2.0/(zFar - zNear);

	M(3,0) = - (right + left) / (right - left);
	M(3,1) = - (top + bottom) / (top - bottom);
	M(3,2) = - (zFar + zNear) / (zFar - zNear);

	// M(3,3) = 1 :P
	return M.as_mat4();
}

mat4 mat4::proj_persp(float left, float right, float bottom, float top, float zNear, float zFar) {
		
	float_arr_mat4 M(mat4::identity());

	M(0,0) = (2*zNear)/(right-left);
	
	M(1,1) = (2*zNear)/(top-bottom);
	
	M(2,0) = (right+left)/(right-left);
	M(2,1) = (top+bottom)/(top-bottom);
	M(2,2) = -(zFar + zNear)/(zFar - zNear);
	M(2,3) = -1.0;
	
	M(3,2) = -(2*zFar*zNear)/(zFar - zNear);
	
	return M.as_mat4();

}

// acts like a gluPerspective call :P

mat4 mat4::proj_persp(float fov_radians, float aspect, float zNear, float zFar) {

	// used glm as a reference

	const float range = tan(fov_radians/2.0) * zNear;
	const float left = -range * aspect;
	const float right = range * aspect;

	return mat4::proj_persp(left, right, -range, range, zNear, zFar);
}

// performs an "in-place transposition" of the matrix

void mat4::transpose() {

	_MM_TRANSPOSE4_PS(data[0], data[1], data[2], data[3]);	// microsoft special in-place transpose macro :P

	/* Implemented as follows: (xmmintrin.h) (no copyright though)
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
	*/

}

mat4 mat4::transposed() const {

	mat4 ret = (*this);	
	_MM_TRANSPOSE4_PS(ret.data[0], ret.data[1], ret.data[2], ret.data[3]);
	return ret;

}


void mat4::invert() {
	mat4 i = this->inverted();
	memcpy(data, &i.data, sizeof(i.data));
}

mat4 mat4::inverted() const {

	mat4 m = this->transposed();
	__m128 minor0, minor1, minor2, minor3;
	// _mm_shuffle_pd(a, a, 1) can be used to swap the hi/lo qwords of a __m128d,
	// and _mm_shuffle_ps(a, a, 0x4E) for a __m128

	__m128	row0 = m.data[0],
			row1 = _mm_shuffle_ps(m.data[1], m.data[1], mask1032),
			row2 = m.data[2],
			row3 = _mm_shuffle_ps(m.data[3], m.data[3], mask1032);

	__m128 det, tmp1;

	//const float *src = (const float*)this->rawData();

	// The following code fragment was copied (with minor adaptations of course)
	// from http://download.intel.com/design/PentiumIII/sml/24504301.pdf.
	// Other alternatives include https://github.com/LiraNuna/glsl-sse2/blob/master/source/mat4.h#L324,
	// but based on pure intrinsic call count, the intel version is more concise (84 vs. 102, many of which shuffles)

	tmp1 = _mm_mul_ps(row2, row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor0 = _mm_mul_ps(row1, tmp1);
	minor1 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
	minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
	minor1 = _mm_shuffle_ps(minor1, minor1, 0x4E);


	tmp1 = _mm_mul_ps(row1, row2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
	minor3 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
	minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
	minor3 = _mm_shuffle_ps(minor3, minor3, 0x4E);


	tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	row2 = _mm_shuffle_ps(row2, row2, 0x4E);
	minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
	minor2 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
	minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
	minor2 = _mm_shuffle_ps(minor2, minor2, 0x4E);


	tmp1 = _mm_mul_ps(row0, row1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
	minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
	minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));


	tmp1 = _mm_mul_ps(row0, row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
	minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
	minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));


	tmp1 = _mm_mul_ps(row0, row2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
	minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
	minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);
	
	det = _mm_mul_ps(row0, minor0);
	det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
	det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);
	tmp1 = _mm_rcp_ss(det);
	det = _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
	det = _mm_shuffle_ps(det, det, 0x00);
	
	minor0 = _mm_mul_ps(det, minor0);
	m.data[0] = minor0;
	
	minor1 = _mm_mul_ps(det, minor1);
	m.data[1] = minor1;
	
	minor2 = _mm_mul_ps(det, minor2);
	m.data[2] = minor2;
	
	minor3 = _mm_mul_ps(det, minor3);
	m.data[3] = minor3;

	return m;
}


float det(const mat4 &m) {

	__m128 minor0;

	__m128	row0 = m.data[0],
			row1 = _mm_shuffle_ps(m.data[1], m.data[1], mask1032),
			row2 = m.data[2],
			row3 = _mm_shuffle_ps(m.data[3], m.data[3], mask1032);

	__m128 det, tmp1;
	
	// just took the intel inverse routine, recursively identified the dependencies of "__m128 det",
	// and stripped any assignments that didn't involve those dependencies :P
	// that said, i'm "ninety-nine point nine nine nine nine nine NINE percent sure" this 
	// isn't the fastest way to compute a 4x4 determinant

	tmp1 = _mm_mul_ps(row2, row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor0 = _mm_mul_ps(row1, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);

	tmp1 = _mm_mul_ps(row1, row2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));

	tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	row2 = _mm_shuffle_ps(row2, row2, 0x4E);
	minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
	
	tmp1 = _mm_mul_ps(row0, row2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	
	det = _mm_mul_ps(row0, minor0);
	det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
	det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);

	return get_first_field(det);

}

mat4 mat4::scale(float x, float y, float z) {
	// assign to main diagonal
	float_arr_mat4 m(mat4::identity());
	m(0, 0) = x;
	m(1, 1) = y;
	m(2, 2) = z;

	return m.as_mat4();
}
mat4 mat4::translate(float x, float y, float z) {
	mat4 m = mat4::identity();
	m.assignToColumn(3, vec4(x, y, z, 1.0));	// the w component isn't usually used in translation
	return m;
}

mat4 mat4::translate(const vec4 &v) {
	mat4 m = mat4::identity();
	m.assignToColumn(3, v);
	return m;
}


mat4 abs(const mat4 &m) {
	// might seem a bit funny, but it's actually used in the OBB SAT cdetection code
	static const __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
	return mat4(vec4(_mm_andnot_ps(mask, m.column(0).getData())),
				vec4(_mm_andnot_ps(mask, m.column(1).getData())),
				vec4(_mm_andnot_ps(mask, m.column(2).getData())),
				vec4(_mm_andnot_ps(mask, m.column(3).getData()))
				);
}

// i'm not quite sure why anybody would ever want to construct a quaternion like this :-XD
Quaternion::Quaternion(float x, float y, float z, float w) { 
	data = _mm_set_ps(w, z, y, x);	// in reverse order
}

Quaternion::Quaternion() { data=QUAT_NO_ROTATION; }

Quaternion Quaternion::conjugate() const {
	return Quaternion(_mm_mul_ps(this->data, QUAT_CONJUGATE));
}

Quaternion Quaternion::fromAxisAngle(float x, float y, float z, float angle_radians) {

	Quaternion q;
	const float half_angle = angle_radians/2;
	const __m128 sin_half_angle = _mm_set1_ps(sin(half_angle));
	
	vec4 axis(x, y, z, 0);
	
	//std::cerr << "\n axis.getData() before normalize: " << axis.getData();
	axis.normalize();

	//std::cerr << "\n axis.getData(): " << axis.getData();

	q.data = _mm_mul_ps(sin_half_angle, axis.getData());
	assign_to_field(q.data, Q::w, cos(half_angle));
	
	return q;
}


std::ostream &operator<< (std::ostream& out, const Quaternion &q) {
	out << q.getData();
	return out;
}

void Quaternion::normalize() { 

	Quaternion &Q = (*this);
	//const float mag_squared = _mm_dp_ps(Q.data, Q.data, xyzw_dot_mask).m128_f32[0];
	const float mag_squared = MM_DPPS_XYZW(Q.data, Q.data);
	if (fabs(mag_squared-1.0) > 0.001) {	// to prevent calculations from exploding
		Q.data = _mm_mul_ps(Q.data, _mm_set1_ps(1.0/sqrt(mag_squared)));
	}

}

Quaternion Quaternion::operator*(const Quaternion &b) const {

	// q1*q2 = q3
	// q3 = v3 + w3,
	// v = w1w2 - dot(v1, v2), 
	// w = w1v2 + w2v1 + cross(v1, v2)

	const Quaternion &a = (*this);

	Quaternion ret(
	_mm_sub_ps(
	_mm_mul_ps(_mm_shuffle_ps(a.data, a.data, mask3021), _mm_shuffle_ps(b.data, b.data, mask3102)),
	_mm_mul_ps(_mm_shuffle_ps(a.data, a.data, mask3102), _mm_shuffle_ps(b.data, b.data, mask3021))));
	
	ret += a(Q::w)*b + b(Q::w)*a;

	//ret(Q::w) = a.data.m128_f32[Q::w]*b.data.m128_f32[Q::w] - _mm_dp_ps(a.data, b.data, xyz_dot_mask).m128_f32[0];
	//ret(Q::w) = a.element(Q::w) * b.element(Q::w) - MM_DPPS_XYZ(a.data, b.data);
	ret.assign(Q::w, a(Q::w) * b(Q::w) - MM_DPPS_XYZ(a.data, b.data));
	return ret;

}

void Quaternion::operator*=(const Quaternion &b) {
	(*this) = (*this) * b;
}

Quaternion Quaternion::operator*(float scalar) const {
	return Quaternion(_mm_mul_ps(_mm_set1_ps(scalar), data));
}

void Quaternion::operator*=(float scalar) {
	this->data = _mm_mul_ps(_mm_set1_ps(scalar), data);
}


Quaternion Quaternion::operator+(const Quaternion& b) const {
	return Quaternion(_mm_add_ps(this->data, b.data));
}

void Quaternion::operator+=(const Quaternion &b) {
	this->data = _mm_add_ps(this->data, b.data);
}

mat4 Quaternion::toRotationMatrix() const {
	
	// ASSUMING *this is a NORMALIZED QUATERNION!

	// the product-of-two-matrices implementation is actually 
	// just barely faster than the original version (below) auto-vectorized by msvc
	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/
	
	
	const __m128 qdata = this->data;
	const __m128 C0 = _mm_shuffle_ps(qdata, qdata, 0x1B),
			  C1 = _mm_shuffle_ps(qdata, qdata, 0x4E),
			  C2 = _mm_shuffle_ps(qdata, qdata, 0xB1),
			  C3 = qdata;

#define PLUS 0x00000000	
#define MINUS 0x80000000	// a = -a requires flipping the sign bit, so xor sign bit with 1
	static const __m128 sgnmaskL0 = _mm_castsi128_ps(_mm_setr_epi32(PLUS, MINUS, PLUS, MINUS));
	static const __m128 sgnmaskL1 = _mm_castsi128_ps(_mm_setr_epi32(PLUS, PLUS, MINUS, MINUS));
	static const __m128 sgnmaskL2 = _mm_castsi128_ps(_mm_setr_epi32(MINUS, PLUS, PLUS, MINUS));
	static const __m128 sgnmaskL3 = _mm_castsi128_ps(_mm_setr_epi32(PLUS, PLUS, PLUS, PLUS));

	static const __m128 sgnmaskR0 = _mm_castsi128_ps(_mm_setr_epi32(PLUS, MINUS, PLUS, PLUS));
	static const __m128 sgnmaskR1 = _mm_castsi128_ps(_mm_setr_epi32(PLUS, PLUS, MINUS, PLUS));
	static const __m128 sgnmaskR2 = _mm_castsi128_ps(_mm_setr_epi32(MINUS, PLUS, PLUS, PLUS));
	static const __m128 sgnmaskR3 = _mm_castsi128_ps(_mm_setr_epi32(MINUS, MINUS, MINUS, PLUS));

	mat4 res = mat4(_mm_xor_ps(C0, sgnmaskL0),
					_mm_xor_ps(C1, sgnmaskL1),
					_mm_xor_ps(C2, sgnmaskL2),
					C3) //_mm_xor_ps(C3, sgnmaskL3)) // sgnmaskL3 is all zeroes so no need to xor
				*
				mat4(_mm_xor_ps(C0, sgnmaskR0),
					 _mm_xor_ps(C1, sgnmaskR1),
					 _mm_xor_ps(C2, sgnmaskR2),
					 _mm_xor_ps(C3, sgnmaskR3));
	return res; 
	
	/*
	const Quaternion &q = *this;
	ALIGNED16(float tmp[4]);
	_mm_store_ps(tmp, this->data);
	const float x2 = tmp[Q::x]*tmp[Q::x];
	const float y2 = tmp[Q::y]*tmp[Q::y];
	const float z2 = tmp[Q::z]*tmp[Q::z];
	const float xy = tmp[Q::x]*tmp[Q::y];
	const float xz = tmp[Q::x]*tmp[Q::z];
	const float xw = tmp[Q::x]*tmp[Q::w];
	const float yz = tmp[Q::y]*tmp[Q::z];
	const float yw = tmp[Q::y]*tmp[Q::w];
	const float zw = tmp[Q::z]*tmp[Q::w];

	return mat4(vec4(1.0 - 2.0*(y2 + z2), 2.0*(xy-zw), 2.0*(xz + yw), 0.0f),
				vec4(2.0 * (xy + zw), 1.0 - 2.0*(x2 + z2), 2.0*(yz - xw), 0.0),
				vec4(2.0 * (xz - yw), 2.0 * (yz + xw), 1.0 - 2.0 * (x2 + y2), 0.0),
				vec4(0.0, 0.0, 0.0, 1.0)); */
	
}

Quaternion operator*(float scalar, const Quaternion& q) {
	return Quaternion(_mm_mul_ps(_mm_set1_ps(scalar), q.getData()));
}

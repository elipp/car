#include "lin_alg.h"
#include "text.h"

static const __m128 ZERO = _mm_setzero_ps();
static const __m128 MINUS_ONES = _mm_set_ps(-1.0, -1.0, -1.0, -1.0);
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
								

const vec4 vec4::zero_const = vec4(ZERO);
const mat4 identity_const_mat4 = mat4(identity_arr);
const mat4 zero_const_mat4 = mat4(zero_arr);


static float MM_DPPS_XYZ_SSE(__m128 a, __m128 b) {
	const __m128 mul = _mm_mul_ps(a, b);
	//const __m128 t1 = _mm_movehl_ps(mul, mul);
	//const __m128 t2 = _mm_add_ps(mul, t1);
	//const __m128 sum = _mm_add_ss(t1, _mm_shuffle_ps(t2, t2, 1));
	//return get_first_field(sum);
	__declspec(align(16)) float tmp[4];
	_mm_store_ps(tmp, mul);
	return tmp[0] + tmp[1] + tmp[2];
}

inline float MM_DPPS_XYZW_SSE(__m128 a, __m128 b) {
	const __m128 mul = _mm_mul_ps(a, b);
	const __m128 t = _mm_add_ps(mul, _mm_movehl_ps(mul, mul));
	const __m128 sum = _mm_add_ss(t, _mm_shuffle_ps(t, t, 1));
	return get_first_field(sum);

}

inline float MM_DPPS_XYZ_SSE3(__m128 a, __m128 b) {
	__m128 mul = _mm_mul_ps(a, b);	
	 assign_to_field(mul, 3, 0); // mul = (ax*bx, ay*by, az*bz, 0)
	 __m128 t = _mm_hadd_ps(mul, mul);	// t = (ax*bx + ay*by, az*bz + 0, ax*bx + ay*by, az*bz + 0)
	 t = _mm_hadd_ps(t, t);				// t = (ax*bx + ay*by + az*bz + 0, ax*bx + ay*by + az*bz + 0, sim., sim.)
	 return get_first_field(t);
}

inline float MM_DPPS_XYZW_SSE3(__m128 a, __m128 b) {
	__m128 mul = _mm_mul_ps(a, b);
	__m128 t = _mm_hadd_ps(mul, mul);
	t = _mm_hadd_ps(t, t);
	return get_first_field(t);
}

inline float MM_DPPS_XYZ_SSE41(__m128 a, __m128 b) {
	return get_first_field(_mm_dp_ps(a, b, xyz_dot_mask));
}

inline float MM_DPPS_XYZW_SSE41(__m128 a, __m128 b) {
	return get_first_field(_mm_dp_ps(a, b, xyzw_dot_mask));
}

float MM_DPPS_XYZ(__m128 a, __m128 b) {
	// http://stackoverflow.com/questions/6996764/fastest-way-to-do-horizontal-float-vector-sum-on-x86 ^^
	return MM_DPPS_XYZ_SSE(a, b);
}

float MM_DPPS_XYZW(__m128 a, __m128 b) {
	return MM_DPPS_XYZW_SSE(a, b);
}

const char* checkCPUCapabilities() {

	typedef struct _cpuid_t {
		int eax, ebx, ecx, edx;
	} cpuid_t;

	cpuid_t c;
	memset(&c, 0, sizeof(c));
#ifdef _WIN32
	__cpuid((int*)&c, 1);
#elif __linux__
	cpuid(0x1, c);
#endif

	// would be better to somehow check these flags at compile time

#define SSE_BIT_ENABLED ((c.edx & 0x02000000) == 0x02000000) // register edx, bit 25
#define SSE3_BIT_ENABLED ((c.ecx & 0x00000001) == 0x00000001) // register ecx, bit 0
#define SSE41_BIT_ENABLED ((c.ecx & 0x00080000) == 0x00080000) // register ecx, bit 19
	
	if (!SSE_BIT_ENABLED) {
		return "ERROR: SSE not supported by host processor!";
	}
	/*
	else if (!SSE3_BIT_ENABLED) {
		MM_DPPS_XYZ = MM_DPPS_XYZ_SSE;
		MM_DPPS_XYZW = MM_DPPS_XYZW_SSE;
		return "NOTE: lin_alg: SSE3 not supported by host processor. Using SSE(1) for dot product computation.\n";
	}
	
	else if (!SSE41_BIT_ENABLED) {
		MM_DPPS_XYZ = MM_DPPS_XYZ_SSE3;
		MM_DPPS_XYZW = MM_DPPS_XYZW_SSE3;
		return "NOTE: lin_alg: SSE4.1 not supported by host processor. Using SSE3 hadd for dot product computation.\n";
	}
	else { 
		MM_DPPS_XYZ = MM_DPPS_XYZ_SSE41;
		MM_DPPS_XYZW = MM_DPPS_XYZW_SSE41;
		return "NOTE: lin_alg: Using SSE4.1 for dot product computation (_mm_dp_ps, optimal).\n";
	}
	*/
	return "Using SSE1 for dot product computation.\n";
}

vec4::vec4(float _x, float _y, float _z, float _w) {
	// must use _mm_setr_ps for this constructor. Note 'r' for reversed.
	//data = _mm_setr_ps(_x, _y, _z, _w);
	data = _mm_set_ps(_w, _z, _y, _x);	// could be faster, haven't tested

}


vec4::vec4() {
	data = ZERO;
}

// obviously enough, this requires a 4-float array as argument
vec4::vec4(const float* const a) {
	data = _mm_loadu_ps(a);		// not assuming 16-byte alignment for a.
}

void vec4::operator*=(float scalar) {

	// use xmmintrin
	// data = _mm_mul_ps(data, _mm_load1_ps(&scalar)); // not quite sure this works
	data = _mm_mul_ps(data, _mm_set1_ps(scalar));	// the disassembly is pretty much identical between the two though
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

float vec4::length3() const {
	//return sqrt(_mm_dp_ps(this->data, this->data, xyz_dot_mask).m128_f32[0]);
	return sqrt(MM_DPPS_XYZ(this->data, this->data));

}

float vec4::length4() const {

	//return sqrt(_mm_dp_ps(this->data, this->data, xyzw_dot_mask).m128_f32[0]);	// includes x,y,z,w in the computation
	return sqrt(MM_DPPS_XYZW(this->data, this->data));

}

// this should actually include all components, but given the application, this won't :P

void vec4::normalize() {

	// 0.08397us/iteration for this
	//const float l_recip = 1.0/sqrt(_mm_dp_ps(this->data, this->data, xyz_dot_mask).m128_f32[0]); // only x,y,z components
	//this->data = _mm_mul_ps(this->data, _mm_set1_ps(l_recip));

	// 0.07257us/iteration, probably has a bigger error margin though
	//const __m128 l_recip = _mm_rcp_ps(_mm_sqrt_ps(_mm_set1_ps(MM_DPPS_XYZ(this->data, this->data)))); // only x,y,z components
	//this->data = _mm_mul_ps(this->data, l_recip);

	// in debug mode (no optimization whatsoever), stripping the temporary value improved
	// performance to about 0.06786us/iteration
	//this->data = _mm_mul_ps(this->data, _mm_rcp_ps(_mm_sqrt_ps(_mm_set1_ps(MM_DPPS_XYZ(this->data, this->data)))));
	const float dot3 = MM_DPPS_XYZ(this->data, this->data);
	const __m128 factor = _mm_rsqrt_ps(_mm_set1_ps(dot3));	// rsqrtps = approximation :P
	//const __m128 orig_data = this->data;
	this->data = _mm_mul_ps(factor, this->data);
	//std::cerr << "\nat vec4::normalize: factor = " << factor << "\norig data = " << orig_data << "\n data = " << this->data << ", dot3 = " << dot3 << "\n";


}

vec4 vec4::normalized() const {
	vec4 v(*this);
	v.normalize();
	return v;
}

void vec4::zero() {
	(*this) = vec4::zero_const;
}

std::ostream &operator<< (std::ostream& out, const vec4 &v) {
	out << v.getData();
	return out;
}

void* vec4::rawData() const {
	return (void*)&data;
}


float dot(const vec4 &a, const vec4 &b) {
	return MM_DPPS_XYZ(a.data, b.data);
}

vec4 cross(const vec4 &a, const vec4 &b) {

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
	mat4 M(MAT_IDENTITY);
	M.assignToColumn(3, (*this));
	return M;
}


mat4::mat4() {
	data[0] = data[1] = data[2] = data[3] = ZERO;
}

mat4::mat4(const float *arr) {
	data[0] = _mm_loadu_ps(arr);
	data[1] = _mm_loadu_ps(arr + 4);	
	data[2] = _mm_loadu_ps(arr + 8);	
	data[3] = _mm_loadu_ps(arr + 12);	

}

mat4::mat4(const int main_diagonal_val) {
	mat4 &a = (*this);	// reference (*this) for easier manipulation
	a = mat4::zero();
	a.assign(0, 0, main_diagonal_val);
	a.assign(1, 1, main_diagonal_val);
	a.assign(2, 2, main_diagonal_val);
	a.assign(3, 3, main_diagonal_val);

}


// stupid constructor.

mat4::mat4(const vec4& c1, const vec4& c2, const vec4& c3, const vec4& c4) {

	data[0] = c1.getData();
	data[1] = c2.getData();
	data[2] = c3.getData();
	data[3] = c4.getData();

}

// must be passed as references lolz, otherwise the last one will lose its alignment

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

	mat4 L = (*this).transposed();
	mat4 ret;
	
	// we'll choose to transpose the other matrix, and instead of calling the perhaps
	// more intuitive row(), we'll be calling column(), which is a LOT faster in comparison.
	
	// FIXME: too many assign():s! will be very slow

	__declspec(align(16)) float tmp[4];	// represents a temporary column

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			tmp[j] = MM_DPPS_XYZW(L.data[j], R.data[i]);
		}
		ret.assignToColumn(i, _mm_load_ps(tmp));
	}
	
	return ret;

}


vec4 mat4::operator* (const vec4& R) const {

	
	// try with temporary mat4? :P
	// result: performs better (with optimizations disabled at least)
	const mat4 M = (*this).transposed();
	__declspec(align(16)) float tmp[4];
	for (int i = 0; i < 4; i++) {
		tmp[i] = MM_DPPS_XYZW(M.data[i], R.getData());
	}

	return vec4(tmp);
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

	return vec4(this->transposed().data[i]);
	// benchmarks for 100000000 iterations
	// Two transpositions, no redirection to mat4::column:	15.896s
	// Two transpositions, redirection to mat4::column:		18.332s
	// Naive implementation:								14.140s. !
	// copy, transpose:										11.354s
	
	// for comparison: column, 100000000 iterations:		4.7s

}

vec4 mat4::column(int i) const {
	return vec4(this->data[i]);
}

void mat4::assignToColumn(int column, const vec4& v) {
	this->data[column] = v.getData();
}

void mat4::assignToRow(int row, const vec4& v) {
	// here, since 
	// 1. row operations are inherently slower than column operations, and
	// 2. transposition is blazing fast (:D)

	// we could just transpose the matrix and do some fancy sse shit with it.
	
	// this could (and probably should) be done with a reference, like this:
	// mat4.row(i) = vec4(...). However, this is only possible if the mat4 is 
	// constructed of actual vec4s (which is something one should consider anyway)

	this->transpose();
	this->data[row] = v.getData();
	this->transpose();

}
// return by void pointer? :P
void *mat4::rawData() const {
	return (void*)&data[0];	// seems to work just fine :D
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
		
	mat4 M = mat4::identity();
	
	//M(0,0) = 2.0/(right - left);
	M.assign(0, 0, 2.0/(right - left));

	//M(1,1) = 2.0/(top - bottom);
	M.assign(1, 1, 2.0/(top - bottom));
	
//	M(2,2) = -2.0/(zFar - zNear);
	M.assign(2, 2, -2.0/(zFar - zNear));

	//M(3,0) = - (right + left) / (right - left);
	M.assign(3, 0, -(right + left) / (right - left));

	//M(3,1) = - (top + bottom) / (top - bottom);
	M.assign(3, 1, - (top + bottom) / (top - bottom));

//	M(3,2) = - (zFar + zNear) / (zFar - zNear);
	M.assign(3, 2, -(zFar + zNear) / (zFar - zNear));
	// the element at (3,3) is already 1 (identity() was called)
	return M;
}

mat4 mat4::proj_persp(float left, float right, float bottom, float top, float zNear, float zFar) {
		
	mat4 M = mat4::identity();

	// FIXME: operate on a __declspec(align(16)) float[16], do assignments, construct mat4. 

	//M(0,0) = (2*zNear)/(right-left);
	M.assign(0, 0, (2*zNear)/(right-left));
	
	//M(1,1) = (2*zNear)/(top-bottom);
	M.assign(1, 1, (2*zNear)/(top-bottom));

//	M(2,0) = (right+left)/(right-left);
	M.assign(2, 0, (right+left)/(right-left));

//	M(2,1) = (top+bottom)/(top-bottom);
	M.assign(2, 1, (top+bottom)/(top-bottom));
	
	//M(2,2) = -(zFar + zNear)/(zFar - zNear);
	M.assign(2, 2, -(zFar + zNear)/(zFar - zNear));

	//M(2,3) = -1.0;
	M.assign(2, 3, -1.0);
	
	//M(3,2) = -(2*zFar*zNear)/(zFar - zNear);
	M.assign(3, 2, -(2*zFar*zNear)/(zFar - zNear));
	
	return M;

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

	mat4 ret = (*this);	// copying can't be avoided
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


inline float det(const mat4 &m) {
	return 0.0; // :*(
}

mat4 mat4::scale(float x, float y, float z) {
	// assign to main diagonal
	mat4 m;
	m.assign(0, 0, x);
	m.assign(1, 1, y);
	m.assign(2, 2, z);
	m.assign(3, 3, 1);
	return m;
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
	assign_to_field(ret.data, Q::w, a(Q::w) * b(Q::w) - MM_DPPS_XYZ(a.data, b.data));
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

// a name such as "applyQuaternionRotation" would be much more fitting,
// since quaternion-vector multiplication doesn't REALLY exist

vec4 Quaternion::operator*(const vec4& b) const {

	vec4 v(b);
	v.normalize();
 	Quaternion vec_q(b.getData()), res_q;
	//vec_q(Q::w) = 0.0;
	assign_to_field(vec_q.data, Q::w, 0.0);

	res_q = vec_q * (*this).conjugate();
	res_q = (*this)*res_q;
	
	return vec4(res_q.data);	// the w component probably contains some garbage, but it's not used anyway
}

mat4 Quaternion::toRotationMatrix() const {
	
	// ASSUMING *this is a NORMALIZED QUATERNION!

	const Quaternion &q = *this;

	__declspec(align(16)) float tmp[4];
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
				vec4(0.0, 0.0, 0.0, 1.0));

}

Quaternion operator*(float scalar, const Quaternion& q) {
	return Quaternion(_mm_mul_ps(_mm_set1_ps(scalar), q.getData()));
}

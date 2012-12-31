#include "lin_alg.h"

vec4 operator*(float f, vec4 v) {
	return vec4(f*v.x, f*v.y, f*v.z, f*v.w);
}



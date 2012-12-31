#ifndef LIN_ALG_H
#define LIN_ALG_H

class vec4 {
	public:
	float x, y, z, w;
	vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
	vec4() { x = y = z = w = 0; }
	float length() { return sqrt(x*x + y*y + z*z + w*w); }
	void normalize() { float l = length();
		x /= l;
		y /= l;
		z /= l;
		w /= l;
	}
	vec4 operator*(float f) { vec4 v; v.x=x*f; v.y=y*f; v.z=z*f; v.w=w*f; return v; }
	void operator*=(float f) { x*=f; y*=f; z*=f; w*=f; }
	vec4 operator+(vec4 b) { return vec4(x + b.x, y + b.y, z + b.z, w + b.w); }
	void operator+=(vec4 b) { x += b.x; y += b.y; z += b.z; w += b.w; }
	vec4 operator-(vec4 b) { return vec4(x - b.x, y - b.y, z - b.z, w - b.z); }
	void operator-=(vec4 b) { x -= b.x; y -= b.y; z -= b.z; w -= b.w; }

	void print() { 
		std::cout << "(" << x << ", " << y << ", " << z << ", " << w << ")\n";
	}
};

vec4 operator*(float f, vec4 v);

class mat4 {
public:
	float data[16];
	mat4(float* _data) { memcpy(data, _data, sizeof(data)); }
	mat4() { memset(data, 0, sizeof(data)); }

	float& operator()(int col, int row) {
		return data[col*4 + row];
	}

	vec4 operator*(vec4 V) {
		vec4 res;
		mat4 M = (*this);
		res.x = M(0,0)*V.x + M(0,1)*V.y + M(0,2)*V.z + M(0,3)*V.w;
		res.y = M(1,0)*V.x + M(1,1)*V.y + M(1,2)*V.z + M(1,3)*V.w;
		res.z = M(2,0)*V.x + M(2,1)*V.y + M(2,2)*V.z + M(2,3)*V.w;
		res.w = M(3,0)*V.x + M(3,1)*V.y + M(3,2)*V.z + M(3,3)*V.w;
		return res;
	}
};


#endif

#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <map>

#include <sys/time.h>

#include "lin_alg.h"

#define KEY_UP (0x1 << 0)
#define KEY_DOWN (0x1 << 1)
#define KEY_LEFT (0x1 << 2)
#define KEY_RIGHT (0x1 << 3)

typedef std::map<const int, struct client> id_client_map;
typedef std::pair<const int, struct client> id_client_pair;
typedef std::pair<id_client_map::iterator, bool> id_client_it;

struct ip_port_struct { 
	const char* address;
	unsigned int port;
};

struct vertex {
	float vx, vy, vz;
	float nx, ny, nz;
	float u, v;
};

struct car_serialized {
	float position[4];
	float direction;
	float fw_angle;
	float susp_angle_roll;
	float susp_angle_fwd;
	float velocity;
};

class _timer {

	struct timespec beg;
	struct timespec end;

public:
	
	void begin() {
		clock_gettime(CLOCK_REALTIME, &beg);
	}
	time_t get_ns() {
		clock_gettime(CLOCK_REALTIME, &end);
		return (end.tv_sec*1000000000 + end.tv_nsec) - (beg.tv_sec*1000000000 + beg.tv_nsec);
	}
	time_t get_us() {
		clock_gettime(CLOCK_REALTIME, &end);
		return ((end.tv_sec*1000000000 + end.tv_nsec) - (beg.tv_sec*1000000000 + beg.tv_nsec))/1000;
	}
	time_t get_ms() {
		clock_gettime(CLOCK_REALTIME, &end);
		return ((end.tv_sec*1000000000 + end.tv_nsec) - (beg.tv_sec*1000000000 + beg.tv_nsec))/1000000;
	}
	_timer() { memset(&beg, 0, sizeof(struct timespec)); memset(&end, 0, sizeof(struct timespec)); }

};

class car {
public:
	vec4 position;
	float tmpx;
	float direction;	// an angle in the x:z -plane
	float fw_angle;
	float susp_angle_roll;
	float susp_angle_fwd;
	float F_centripetal;
	float velocity;
	
	car() {
		position = vec4(0.0, 0.0, -9.0, 0.0);
		tmpx = direction = fw_angle = susp_angle_roll = susp_angle_fwd = F_centripetal = velocity = 0;
	}
	struct car_serialized serialize() { 
		struct car_serialized r;
		r.position[0] = position.x;
		r.position[1] = position.y;
		r.position[2] = position.z;
		r.position[3] = position.w;
		r.direction = direction;
		r.fw_angle = fw_angle;
		r.susp_angle_roll = susp_angle_roll;
		r.susp_angle_fwd = susp_angle_fwd;
		r.velocity = velocity;
		return r;
	}

	void update_from_serialized(const struct car_serialized& s) {
		direction = s.direction;
		position = vec4(s.position[0], s.position[1], s.position[2], s.position[3]);
		fw_angle = s.fw_angle;
		susp_angle_roll = s.susp_angle_roll;
		susp_angle_fwd = s.susp_angle_fwd;
		velocity = s.velocity;
	}
};

std::string int_to_string(int i);
int string_to_int(const std::string &string);

float f_wheel_angle(float x);

std::vector<std::string> tokenize(const std::string& str, char delim, unsigned tmax = 0xFFFFFFFF);

#endif

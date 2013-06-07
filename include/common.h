#ifndef COMMON_H
#define COMMON_H

#include <Winsock2.h>
#include <Windows.h>

#include <fstream>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sstream>

#include "lin_alg.h"
#include "glwindow_win32.h"

#ifndef M_PI
#define M_PI 3.14159265359
#endif

extern std::string get_timestamp();

static const float PI_PER_TWO = M_PI/2;

static const float PI_PER_180 = (M_PI/180.0);
static const float _180_PER_PI = (180.0/M_PI);

inline float RADIANS(float angle_in_degrees) { return angle_in_degrees*PI_PER_180; }
inline float DEGREES(float angle_in_radians) { return angle_in_radians*_180_PER_PI; }

std::vector<std::string> split(const std::string &s, char delim);

class Car {
public:
	union {
		struct {
			float _position[3];
			float direction;
			float wheel_rot;
			float susp_angle_roll;
			float velocity;
			float front_wheel_angle;
		} state;

		float data_serial[8];
	};

	// these are used to compute other variables at the server-side, no need to pass them around
	struct {
		float front_wheel_tmpx;
		float F_centripetal;
		float susp_angle_fwd;

	} data_internal;

	vec4 position() const { return vec4(state._position[0], state._position[1], state._position[2], 1.0); }
	
	Car() { memset(this, 0, sizeof(*this)); }
}; 


float f_wheel_angle(float x);

#define GOTO_OFFSET(origin, iter, offset) \
	do { \
	iter = origin + offset; \
	} while(0)

#define GOTO_OFFSET_SUM(origin, iter, increment, sum) \
	do { \
	sum += increment; \
	iter = origin + sum; \
	} while(0)

#define NEXTLINE(iter) \
	while (*iter != '\n') \
		iter++; \
	iter++ \


#ifdef _WIN32
class _timer {
	double cpu_freq;	// in kHz
	__int64 counter_start;

	__int64 get() const {
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		return li.QuadPart;
	}
public:
	bool init() {
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);
		cpu_freq = double(li.QuadPart);	// in Hz. this is subject to dynamic frequency scaling, though
		begin();
		return true;
	}
	void begin() {
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);
		cpu_freq = double(li.QuadPart);
		QueryPerformanceCounter(&li);
		counter_start = li.QuadPart;
	}


	inline double get_s() const {	
		return double(_timer::get()-_timer::counter_start)/_timer::cpu_freq;
	}
	inline double get_ms() const {
		return double(1000*(_timer::get_s()));
	}
	inline double get_us() const {
		return double(1000000*(_timer::get_s()));
	}
	_timer() {
		if (!init()) { fprintf(stderr, "_timer error.\n"); }
	}
};


#elif __linux__
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
		return get_ns()/1000;
	}
	time_t get_ms() {
		return get_ns()/1000000;
	}
	double get_s() {
		return (double)get_ns()/1000000000.0;

	}
	_timer() { memset(&beg, 0, sizeof(struct timespec)); 
		   memset(&end, 0, sizeof(struct timespec)); }

};
#endif



size_t getfilesize(FILE *file);	
size_t cpp_getfilesize(std::ifstream&);

char* decompress_qlz(std::ifstream &file, char** buffer);


#endif

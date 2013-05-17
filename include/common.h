#ifndef COMMON_H
#define COMMON_H

#ifdef _WIN32
#include <Windows.h>
#endif
#include <fstream>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#ifdef __linux__
#include <sys/time.h>
#endif
#define GLEW_STATIC 
#include <GL/glew.h>
#include <GL/gl.h>

#ifndef M_PI
#define M_PI 3.14159265359
#endif

static const float PI_PER_TWO = M_PI/2;

static const float PI_PER_180 = (M_PI/180.0);
static const float _180_PER_PI = (180.0/M_PI);

inline float RADIANS(float angle_in_degrees) { return angle_in_degrees*PI_PER_180; }
inline float DEGREES(float angle_in_radians) { return angle_in_radians*_180_PER_PI; }

void logWindowOutput(const char* format, ...);

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

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
	
	__int64 get() {
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		return li.QuadPart;
	}
public:

	bool init() {
		LARGE_INTEGER li;
		if (!QueryPerformanceFrequency(&li)) {
			logWindowOutput( "_timer: initialization failed.\n");
			return false;
		}
		cpu_freq = double(li.QuadPart);	// in Hz. this is subject to dynamic frequency scaling, though

		return true;
	}
	void begin() {
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		counter_start = li.QuadPart;
	}


	inline double get_s() {	
		return double(_timer::get()-_timer::counter_start)/_timer::cpu_freq;
	}
	inline double get_ms() {
		return double(1000*(_timer::get_s()));
	}
	inline double get_us() {
		return double(1000000*(_timer::get_s()));
	}
	_timer() {
		if (!init()) { logWindowOutput("_timer error.\n"); }
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

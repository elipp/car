#ifndef TEXT_H
#define TEXT_H

#include <string>
#include <cstring>
#include <vector>

#include "glwindow_win32.h"
#include "common.h"
#include "precalculated_texcoords.h"
#include "lin_alg.h"

#define BLANK_GLYPH (sizeof(glyph_texcoords)/(8*sizeof(float)) - 1)

struct vertex2 {		

	float vx, vy;
	float u, v;
	vertex2(float _vx, float _vy, float _u, float _v) : vx(_vx), vy(_vy), u(_u), v(_v) {}
	vertex2() {}

};

struct glyph {
	vertex2 vertices[4];
};

class wpstring_holder;

#define WPSTRING_LENGTH_MAX 256

static const GLuint WPS_DYNAMIC = 0x0, WPS_STATIC = 0x01;

class wpstring {

	friend class wpstring_holder;
	std::string text;
	size_t actual_size;

public:
	int x, y;
	wpstring(const std::string &text_, GLuint x_, GLuint y_);

	bool visible;
	void updateString(const std::string& newtext, int offset, int index);
	std::size_t getActualSize() const { return actual_size; }
};


class wpstring_holder { 

	static GLuint static_VBOid;
	static GLuint dynamic_VBOid;
	static GLuint shared_IBOid;
	static std::size_t static_strings_total_length;
	// some kind of size hinting (reserve) could be done
	static std::vector<wpstring> static_strings;
	static std::vector<wpstring> dynamic_strings;

public:
	static GLuint get_static_VBOid() { return static_VBOid; }
	static GLuint get_dynamic_VBOid() { return dynamic_VBOid; }
	static GLuint get_IBOid() { return shared_IBOid; }
	static void updateDynamicString(int index, const std::string& newtext, int offset = 0);
	static void append(const wpstring& string, const GLuint static_mask);
	static void createBufferObjects();
	static std::size_t get_static_strings_total_length() { return static_strings_total_length; }
	static std::size_t getStaticStringCount() { return static_strings.size(); }
	static std::size_t getDynamicStringCount() { return dynamic_strings.size(); }
	static const wpstring &getDynamicString(int index);

};





#endif

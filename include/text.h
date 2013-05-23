#ifndef TEXT_H
#define TEXT_H

#include <string>
#include <cstring>
#include <vector>
#include <array>
#include <unordered_map>

#include "glwindow_win32.h"
#include "lin_alg.h"
#include "precalculated_texcoords.h"
#include "shader.h"

extern mat4 text_Projection;
extern mat4 text_ModelView;
extern GLuint text_texId;
extern GLuint text_shared_IBOid;

extern ShaderProgram *text_shader;


#define BLANK_GLYPH (sizeof(glyph_texcoords)/(8*sizeof(float)) - 1)

struct vertex2 {		
	float x, y;
	float u, v;
	vertex2(float _x, float _y, float _u, float _v) : x(_x), y(_y), u(_u), v(_v) {}
	vertex2() {}
};

struct glyph {
	vertex2 vertices[4];
};

// void track(T var, const std::string &id, float pos_x, float pos_y);
// void untrack(T var);

#define OSL_BUFFER_SIZE 8096	// only the GLushort-based index buffer poses a limit to this
#define OSL_LINE_LEN 128
#define OSL_NUM_LINES (OSL_BUFFER_SIZE / OSL_LINE_LEN)
#define OSL_LINE_LEN_MINUS_ONE (OSL_LINE_LEN - 1)
#define OSL_NUM_LINES_DISPLAYED 8

// one could consider glScissor for this :D

class onScreenLog {
	static float pos_x, pos_y;	// ze upper left corner
	static mat4 modelview;
	static GLuint VBOid;
	static unsigned line_length;
	static unsigned num_lines_displayed;
	static unsigned current_index;
	static unsigned current_line_num;
	static void generate_VBO();
	static void update_VBO(const char* buffer, unsigned length);
	static bool _visible;
	static void set_y_translation(float new_y);

public:
	static void toggle_visibility() { _visible = !_visible; }
	static void scroll(float ds);
	static void print(const char* format, ...);
	static void clear();
	static void draw();
	static int init();
private:
	onScreenLog();
};



#endif

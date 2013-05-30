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

struct xy { 
	float x; float y;
};

struct vertex2 {		
	float x, y;
	float u, v;
	vertex2(const struct xy pos, const struct uv tc) : x(pos.x), y(pos.y), u(tc.u), v(tc.v) {}
	vertex2() {}
};

struct glyph {
	vertex2 vertices[4];
};

// void track(T var, const std::string &id, float pos_x, float pos_y);
// void untrack(T var);

#define OSL_BUFFER_SIZE 8096	// only the GLushort-based index buffer poses a limit to this
#define OSL_LINE_LEN 64
#define OSL_NUM_LINES (OSL_BUFFER_SIZE / OSL_LINE_LEN)
#define OSL_LINE_LEN_MINUS_ONE (OSL_LINE_LEN - 1)
#define OSL_NUM_LINES_DISPLAYED 8

#include <queue>
#include <mutex>


static std::mutex print_queue_mutex;

class PrintQueue {
public:
	std::mutex mutex;
	std::string queue;
	
	void add(const std::string &s);
	PrintQueue() { 	queue.reserve(OSL_BUFFER_SIZE); queue.clear();	}
};

class onScreenLog {
	static PrintQueue print_queue;	// nice and high-level, but should perhaps just opt for a stringbuffer or something like that instead
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
	static bool _autoscroll;
	static void set_y_translation(float new_y);
	
public:
	static void print_string(const std::string &s);
	static void toggle_visibility() { _visible = !_visible; }
	static void toggle_autoscroll() { _autoscroll = !_autoscroll; }
	static void scroll(float ds);
	static void print(const char* format, ...);
	static void clear();
	static void draw();
	static int init();
	static void dispatch_print_queue();
private:
	onScreenLog() {};
};



#endif

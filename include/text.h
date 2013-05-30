#ifndef TEXT_H
#define TEXT_H

#include <string>
#include <cstring>
#include <vector>
#include <array>
#include <unordered_map>
#include <mutex>

#include "glwindow_win32.h"
#include "lin_alg.h"
#include "precalculated_texcoords.h"
#include "shader.h"

extern mat4 text_Projection;
extern mat4 text_ModelView;
extern GLuint text_texId;
extern GLuint text_shared_IBOid;

extern ShaderProgram *text_shader;
extern GLuint generate_empty_VBO(size_t size, GLint FLAG);

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


#define OSL_BUFFER_SIZE 8096	// only the GLushort-based index buffer poses a limit to this
#define OSL_LINE_LEN 96

class onScreenLog {
public:	
#define INPUT_FIELD_BUFFER_SIZE 256
	static class InputField {
		int cursor_pos;
		std::string input_buffer;
		bool _active;
	public:
		GLuint VBOid;
		bool active() const { return _active; }
		void set_active(bool is_active) { _active = is_active; }
		void insert_char_to_cursor_pos(char c);
		void move_cursor(int amount);
		void draw() const;
		void clear();
		void delete_char_before_cursor_pos();
		void update_VBO();
		void submit_and_parse();	// also does a clear
		InputField() { 
			cursor_pos = 0; input_buffer.reserve(INPUT_FIELD_BUFFER_SIZE); input_buffer.clear(); 
			VBOid = 0;
		}
	
	} input_field;
private:
	static class PrintQueue {
	public:
		std::mutex mutex;
		std::string queue;	// "queue" :D
	
		void add(const std::string &s);
		PrintQueue() { 	queue.reserve(OSL_BUFFER_SIZE); queue.clear();	}
	} print_queue;

	static float pos_x, pos_y;	// ze upper left corner
	static mat4 modelview;
	static GLuint VBOid;
	static unsigned line_length;
	static unsigned num_lines_displayed;
	static unsigned current_index;
	static unsigned current_line_num;
	static unsigned num_characters_drawn;
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

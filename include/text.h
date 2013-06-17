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

extern void text_set_Projection(const mat4 &proj);

extern GLuint text_texId;
extern GLuint text_shared_IBOid;

extern ShaderProgram *text_shader;
extern GLuint generate_empty_VBO(size_t size, GLint FLAG);

extern float char_spacing_vert;
extern float char_spacing_horiz;

#define BLANK_GLYPH (sizeof(glyph_texcoords)/(8*sizeof(float)) - 1)

extern std::string get_timestamp();

struct xy { 
	float x; float y;
};

struct vertex2 {		
	float x, y;
	float u, v;
	vertex2(const struct xy &pos, const struct uv &tc) : x(pos.x), y(pos.y), u(tc.u), v(tc.v) {};
	vertex2(float _x, float _y, const struct uv &tc) : x(_x), y(_y), u(tc.u), v(tc.v) {};
	vertex2() {};
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
		std::string input_buffer;
		glyph glyph_buffer[INPUT_FIELD_BUFFER_SIZE];
		int cursor_pos;

		bool _enabled;
		void update_VBO();
		bool _changed;
	public:
		
		GLuint VBOid;
		float textfield_pos_y;

		bool enabled() const { return _enabled; }
		void enable();
		void disable();
		
		void refresh();

		void update_position() { textfield_pos_y = WINDOW_HEIGHT - char_spacing_vert - 4; }

		void insert_char_to_cursor_pos(char c);
		void delete_char_before_cursor_pos();
		void move_cursor(int amount);

		void draw() const;
		void clear();

		void submit_and_parse();	// also does a clear
		InputField() {
			cursor_pos = 0; 
			input_buffer.reserve(INPUT_FIELD_BUFFER_SIZE); 
			input_buffer.clear(); 
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
	static void update_overlay_pos();
	static void print_string(const std::string &s);
	static void toggle_visibility() { _visible = !_visible; }
	static void scroll(float ds);
	static void print(const char* format, ...);
	static void clear();
	static void draw();
	static int init();
	static void dispatch_print_queue();
private:
	onScreenLog() {};
};

// all this template bullcrap will generate huge amounts of code for us ^^

class TrackableBase {
public:
	const std::string name;
	const std::string unit_string;
	const void *const data;
	virtual std::string print() const = 0;
	TrackableBase(const void* const _data, const std::string &_name, const std::string &_unit) 
		: data(_data), name(_name), unit_string(_unit) {};

};

template <typename T> class Trackable : public TrackableBase {

public:
	Trackable<T>(const T* const data, const std::string &_name, const std::string &_unit = "") 
		: TrackableBase((const void* const)data, _name, _unit) {};
	
	std::string print() const {
		std::ostringstream stream;
		stream.precision(3);
		stream << *(static_cast<const T*>(data)) << unit_string;
		return stream.str();
	}
};

// remember: THE VARIABLES BEING TRACKED MUST HAVE STATIC LIFETIME, else -> guaranteed segfault

#define VarTracker_track(TYPENAME, VARNAME)\
	VarTracker::track(new Trackable<TYPENAME>(&VARNAME,\
	std::string(#TYPENAME) + std::string(" ") + std::string(#VARNAME)))

#define VarTracker_track_unit(TYPENAME, VARNAME, UNIT)\
	VarTracker::track(new Trackable<TYPENAME>(&VARNAME,\
	std::string(#TYPENAME) + std::string(" ") + std::string(#VARNAME),\
	std::string(" ") + (UNIT)))

#define VarTracker_untrack(VARNAME)\
	VarTracker::untrack(&(VARNAME))

class VarTracker {
#define TRACKED_MAX 16
#define TRACKED_LEN_MAX 64
	static float pos_x, pos_y;
	static int cur_total_length;

	static glyph glyph_buffer[TRACKED_MAX*TRACKED_LEN_MAX];
	static GLuint VBOid;
	static std::vector<const TrackableBase* const> tracked;
	static void update_VBO(const std::string &buffer);

public:
	static void init();
	static void update();
	static void draw();
	static void update_position();	// according to window geometry changes
	static void track(const TrackableBase *const var);
	static void untrack(const void *const data_ptr);

};


#endif

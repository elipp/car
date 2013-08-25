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

//#define PRINT_BOTH

#ifdef PRINT_STDERR
#define PRINT(fmt, ...) do {\
	fprintf(stderr, fmt, __VA_ARGS__);\
	} while (0)
#elif defined(PRINT_BOTH)
#define PRINT(fmt, ...) do {\
	fprintf(stderr, fmt, __VA_ARGS__);\
	onScreenLog::print(fmt, __VA_ARGS__);\
	} while (0)
#else
	#define PRINT(fmt, ...) do {\
		onScreenLog::print(fmt, __VA_ARGS__);\
	} while (0)
#endif

enum { TEXT_ATTRIB_ALL = 0 };

extern void text_set_Projection(const mat4 &proj);
extern GLuint text_texId;
extern ShaderProgram *text_shader;

extern float char_spacing_vert;
extern float char_spacing_horiz;

#define BLANK_GLYPH (0x20)

enum { TEXT_COLOR_FG = 0, TEXT_COLOR_RED, TEXT_COLOR_GREEN, TEXT_COLOR_BLUE, TEXT_COLOR_YELLOW, TEXT_COLOR_PURPLE, TEXT_COLOR_TURQ };

extern std::string get_timestamp();

struct char_object {
	union {
		struct {
			unsigned pos_x : 7;	 // pos_x maximum is OSL_LINE_LEN (atm 96; representable in 7 bits).
			unsigned pos_y : 14;
			unsigned char_index : 7;
			unsigned color : 4;
		} bitfields;

		GLuint value;
	} co_union;
};

#define _RGB(r,g,b) ((r)/255.0), ((g)/255.0), ((b)/255.0)
#define _RGBA(r,g,b,a) ((r)/255.0), ((g)/255.0), ((b)/255.0), ((a)/255.0)

#define OSL_BUFFER_SIZE 8096	// only the GLushort-based index buffer poses a limit to this
#define OSL_LINE_LEN 96

class onScreenLog {
public:	
#define INPUT_FIELD_BUFFER_SIZE 256
	static class InputField {
		std::string input_buffer;
		char_object IF_char_object_buffer[INPUT_FIELD_BUFFER_SIZE];
		int cursor_pos;

		bool _enabled;
		void update_VBO();
		bool _changed;
	public:
		
		GLuint IF_VBOid;
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
			IF_VBOid = 0;
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
	static char_object OSL_char_object_buffer[OSL_BUFFER_SIZE];
	static float pos_x, pos_y;	// ze upper left corner
	static mat4 modelview;
	static GLuint OSL_VBOid;
	static unsigned line_length;
	static unsigned num_lines_displayed;
	static unsigned current_index;
	static unsigned current_line_num;
	static unsigned num_characters_drawn;
	static void update_VBO(const std::string &buffer);
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

	static char_object VT_char_object_buffer[TRACKED_MAX*TRACKED_LEN_MAX];
	static GLuint VT_VBOid;
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

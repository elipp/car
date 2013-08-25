#include "text.h"

#include "net/client.h"

mat4 text_Projection; 
GLuint text_texId;

ShaderProgram *text_shader = NULL;

static GLuint text_VAOids[2];
enum { text_VAOid_log = 0, text_VAOid_inputfield = 1 };

static GLuint text_shared_IBOid;
static GLuint vartracker_VAOid;

void text_set_Projection(const mat4 &m) {
	text_Projection = m;
}

float char_spacing_vert = 13.0;
float char_spacing_horiz = 7.0;

onScreenLog::InputField onScreenLog::input_field;
onScreenLog::PrintQueue onScreenLog::print_queue;

#define CURSOR_GLYPH 0x7F	// is really DEL though in utf8

float onScreenLog::pos_x = 7.0, onScreenLog::pos_y = HALF_WINDOW_HEIGHT - 6;
mat4 onScreenLog::modelview;
GLuint onScreenLog::OSL_VBOid = 0;

unsigned onScreenLog::line_length = OSL_LINE_LEN;
unsigned onScreenLog::num_lines_displayed = 32;
unsigned onScreenLog::current_index = 0;
unsigned onScreenLog::current_line_num = 0;
char_object onScreenLog::OSL_char_object_buffer[OSL_BUFFER_SIZE];
bool onScreenLog::_visible = true;
bool onScreenLog::_autoscroll = true;
unsigned onScreenLog::num_characters_drawn = 1;

static float log_bottom_margin = char_spacing_vert*1.55;

#define IS_PRINTABLE_CHAR(c) ((c) >= 0x20 && (c) <= 0x7F)

inline uint8_t texcoord_index_from_char(char c){ 
	return IS_PRINTABLE_CHAR(c) ? ((GLubyte)c - 0x20) : 0;
}


inline char_object char_object_from_char(GLushort x, GLushort y, char ch, GLubyte color) { 
	char_object co;
	co.co_union.bitfields.pos_x = x;
	co.co_union.bitfields.pos_y = y;
	co.co_union.bitfields.char_index = texcoord_index_from_char(ch);
	co.co_union.bitfields.color = color;

	return co;
}

static GLuint generate_empty_VBO(size_t size, GLint FLAG) {
	GLuint VBOid = 0;
	glGenBuffers(1, &VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, VBOid);
	glBufferData(GL_ARRAY_BUFFER, size, NULL, FLAG);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return VBOid;
}

void onScreenLog::InputField::insert_char_to_cursor_pos(char c) {
	if (input_buffer.length() < INPUT_FIELD_BUFFER_SIZE) {
		if (c == VK_BACK) {
			delete_char_before_cursor_pos();
		}
		else {
			input_buffer.insert(input_buffer.begin() + cursor_pos, c);
			move_cursor(1);
		}
	}
	refresh();
	_changed = true;
}

void onScreenLog::InputField::refresh() {
	if (_enabled && _changed) {
		update_VBO();
		_changed = false;
	}
}

void onScreenLog::InputField::delete_char_before_cursor_pos() {
	if (cursor_pos < 1) { return; }
	input_buffer.erase(input_buffer.begin() + (cursor_pos-1));
	move_cursor(-1);
	_changed = true;
}

void onScreenLog::InputField::move_cursor(int amount) {
	cursor_pos += amount;
	cursor_pos = max(cursor_pos, 0);
	cursor_pos = min(cursor_pos, input_buffer.length());
	_changed = true;
}

void onScreenLog::InputField::update_VBO() {

	GLushort x_offset = 0;
	int i = 0;
	for (; i < input_buffer.length(); ++i) {
		IF_char_object_buffer[i] = char_object_from_char(x_offset, 0, input_buffer[i], 0);
		++x_offset;
	}

	IF_char_object_buffer[i] = char_object_from_char(x_offset, 0, CURSOR_GLYPH, 3);
	
	glBindBuffer(GL_ARRAY_BUFFER, onScreenLog::InputField::IF_VBOid);
	glBufferSubData(GL_ARRAY_BUFFER, 0, (input_buffer.length()+1)*sizeof(char_object), (const GLvoid*)&IF_char_object_buffer[0]);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void onScreenLog::draw() {
	
	if (!_visible) { return; }
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glUseProgram(text_shader->getProgramHandle());
	glBindVertexArray(text_VAOids[text_VAOid_log]);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, text_texId);
	text_shader->update_uniform_1i("texture_color", 0);
	
	//static const vec4 overlay_rect_color(0.02, 0.02, 0.02, 0.6);
	//static const vec4 log_text_color(0.91, 0.91, 0.91, 1.0);

	static const mat4 overlay_modelview = mat4::identity();

	text_shader->update_uniform_mat4("Projection", text_Projection);

	//text_shader->update_uniform_vec4("text_color", log_text_color);
	text_shader->update_uniform_mat4("ModelView", onScreenLog::modelview);

	//glEnable(GL_SCISSOR_TEST);
	//glScissor(0, (GLint)1.5*char_spacing_vert, (GLsizei) WINDOW_WIDTH, (GLsizei) num_lines_displayed * char_spacing_vert + 2);

	glDrawArrays(GL_POINTS, 0, (num_characters_drawn-1));
	//fprintf(stderr, "num_chars drawn: %lld\n", (long long)(num_characters_drawn - 1));
	//glDisable(GL_SCISSOR_TEST);

	glBindVertexArray(0);

	input_field.draw();	// it's only drawn if its enabled
	glDisable(GL_BLEND);
}


void onScreenLog::InputField::draw() const {
if (!_enabled) { return; }
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);
	
	glUseProgram(text_shader->getProgramHandle());
	glBindVertexArray(text_VAOids[text_VAOid_inputfield]);
	
	//static const vec4 input_field_text_color(_RGB(243, 248, 111), 1.0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, text_texId);
	text_shader->update_uniform_1i("texture_color", 0);
	//text_shader->update_uniform_vec4("text_color", input_field_text_color);

	static const mat4 InputField_modelview = mat4::translate(vec4(0.0, WINDOW_HEIGHT-char_spacing_vert, 0.0, 1.0));

	text_shader->update_uniform_mat4("ModelView", (const GLfloat*)InputField_modelview.rawData());
	text_shader->update_uniform_mat4("Projection", (const GLfloat*)text_Projection.rawData());
	
	glDrawArrays(GL_POINTS, 0, input_buffer.length()+1);
	glBindVertexArray(0);
}

void onScreenLog::InputField::clear() {
	input_buffer.clear();
	cursor_pos = 0;
	IF_char_object_buffer[0] = char_object_from_char(0, 0, CURSOR_GLYPH, 0);
	update_VBO();
}

void onScreenLog::InputField::enable() {
	if (_enabled == false) {
		clear();
		refresh();
	}
	_enabled=true;
}
void onScreenLog::InputField::disable() {
	if (_enabled == true) {
		clear();
	}
	_enabled = false;
}

void onScreenLog::InputField::submit_and_parse() {
	LocalClient::parse_user_input(input_buffer);
	clear();
}

void onScreenLog::PrintQueue::add(const std::string &s) {
	int excess = queue.length() + s.length() - OSL_BUFFER_SIZE;
	if (excess > 0) {
		dispatch_print_queue();		
	}
	mutex.lock();
	queue.append(s);
	mutex.unlock();
}

void onScreenLog::dispatch_print_queue() {
	print_queue.mutex.lock();
	onScreenLog::print_string(print_queue.queue);
	print_queue.queue.clear();
	print_queue.mutex.unlock();
}

int onScreenLog::init() {
	text_Projection = mat4::proj_ortho(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0, -1.0, 1.0);
	onScreenLog::modelview = mat4::identity();
	OSL_VBOid = generate_empty_VBO(OSL_BUFFER_SIZE*sizeof(char_object), GL_DYNAMIC_DRAW);	

	glGenVertexArrays(2, text_VAOids);
	glBindVertexArray(text_VAOids[text_VAOid_log]);
	
	glEnableVertexAttribArray(TEXT_ATTRIB_ALL);

	glBindBuffer(GL_ARRAY_BUFFER, OSL_VBOid);
	glVertexAttribIPointer(TEXT_ATTRIB_ALL, 1, GL_UNSIGNED_INT, sizeof(char_object), BUFFER_OFFSET(0));


	glBindVertexArray(0);
	glDisableVertexAttribArray(TEXT_ATTRIB_ALL);
	
	input_field.IF_VBOid = generate_empty_VBO(INPUT_FIELD_BUFFER_SIZE*sizeof(char_object), GL_DYNAMIC_DRAW);	

	glBindVertexArray(text_VAOids[text_VAOid_inputfield]);
	glEnableVertexAttribArray(TEXT_ATTRIB_ALL);

	glBindBuffer(GL_ARRAY_BUFFER, input_field.IF_VBOid);
	glVertexAttribIPointer(TEXT_ATTRIB_ALL, 1, GL_UNSIGNED_INT, sizeof(char_object), BUFFER_OFFSET(0));

	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(TEXT_ATTRIB_ALL);

	return 1;
}



void onScreenLog::update_VBO(const std::string &buffer) {

	const size_t length = buffer.length();

	int i = 0;
	static int x_pos = -1, y_pos = 0;
	static int line_beg_index = 0;

	for (i = 0; i < length; ++i) {
		++x_pos;
		char c = buffer[i];

		if (c == '\n' || ((current_index+i)-line_beg_index) >= OSL_LINE_LEN) {
			++current_line_num;
			y_pos = current_line_num;
			x_pos = 0;	
			line_beg_index = (current_index+i);
		}

		OSL_char_object_buffer[i] = char_object_from_char(x_pos, y_pos, c, 0);
	
	}
	
	unsigned prev_current_index = current_index;
	current_index += length;
	
	glBindBuffer(GL_ARRAY_BUFFER, OSL_VBOid);
	if (current_index >= OSL_BUFFER_SIZE - 1) {
		// the excessive part will be flushed to the beginning of the VBO :P
		unsigned excess = current_index - OSL_BUFFER_SIZE + 1;
		unsigned fitting = buffer.length() - excess;
		glBufferSubData(GL_ARRAY_BUFFER, prev_current_index*sizeof(char_object), fitting*sizeof(char_object), (const GLvoid*)OSL_char_object_buffer);
		glBufferSubData(GL_ARRAY_BUFFER, 0, excess*sizeof(char_object), (const GLvoid*)(OSL_char_object_buffer + fitting));
	}
	else {
		glBufferSubData(GL_ARRAY_BUFFER, prev_current_index*sizeof(char_object), length*(sizeof(char_object)), (const GLvoid*)OSL_char_object_buffer);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	/*if (_autoscroll) {
		float d = (y_pos + pos_y + log_bottom_margin) - WINDOW_HEIGHT;
		if (d > onScreenLog::modelview(3,1)) { 
			set_y_translation(-d);
		}
	}*/

}

void onScreenLog::print(const char* fmt, ...) {
	
	char *buffer = new char[OSL_BUFFER_SIZE]; 
	va_list args;
	va_start(args, fmt);

	/*SYSTEMTIME st;
    GetSystemTime(&st);*/

	std::size_t msg_len = vsprintf_s(buffer, OSL_BUFFER_SIZE, fmt, args);

	msg_len = (msg_len > OSL_BUFFER_SIZE - 1) ? OSL_BUFFER_SIZE - 1 : msg_len;
	std::size_t total_len = msg_len;

	buffer[total_len] = '\0';
	buffer[OSL_BUFFER_SIZE-1] = '\0';
	va_end(args);
	
	print_queue.add(buffer);
	num_characters_drawn += total_len;
	num_characters_drawn = (num_characters_drawn > OSL_BUFFER_SIZE) ? OSL_BUFFER_SIZE : (num_characters_drawn);

	delete [] buffer;
}

void onScreenLog::print_string(const std::string &s) {
	update_VBO(s);
}

void onScreenLog::scroll(float ds) {
	float y_adjustment = current_line_num * char_spacing_vert;
	float bottom_scroll_displacement = y_adjustment + log_bottom_margin + pos_y - WINDOW_HEIGHT;
	
	onScreenLog::modelview.assign(3, 1, onScreenLog::modelview(3,1) + ds);
	
	if (bottom_scroll_displacement + onScreenLog::modelview(3,1) >= 0) {
		_autoscroll = false;

	}
	else {		
		set_y_translation(-bottom_scroll_displacement);
		_autoscroll = true;
	}

}

void onScreenLog::set_y_translation(float new_y) {
	onScreenLog::modelview.assign(3, 1, new_y);
}

void onScreenLog::clear() {
	current_index = 1;
	current_line_num = 0;
	num_characters_drawn = 1;	// there's the overlay glyph at the beginning of the vbo =)
	onScreenLog::modelview = mat4::identity();
}


 // *** STATIC VAR DEFS FOR VARTRACKER ***

GLuint VarTracker::VT_VBOid;
std::vector<const TrackableBase* const> VarTracker::tracked;

static int tracker_width_chars = 36;

float VarTracker::pos_x = WINDOW_WIDTH - tracker_width_chars*char_spacing_horiz;
float VarTracker::pos_y = 7;
int VarTracker::cur_total_length = 0;
char_object VarTracker::VT_char_object_buffer[TRACKED_MAX*TRACKED_LEN_MAX];


void VarTracker::init() {
	VarTracker::VT_VBOid = generate_empty_VBO(TRACKED_MAX * TRACKED_LEN_MAX * sizeof(char_object), GL_DYNAMIC_DRAW);
	glGenVertexArrays(1, &vartracker_VAOid);
	
	glBindVertexArray(vartracker_VAOid);
	glEnableVertexAttribArray(TEXT_ATTRIB_ALL);
	glBindBuffer(GL_ARRAY_BUFFER, VarTracker::VT_VBOid);
	glVertexAttribIPointer(TEXT_ATTRIB_ALL, 1, GL_UNSIGNED_INT, sizeof(char_object), BUFFER_OFFSET(0));

	glBindVertexArray(0);
	
	char_object co = char_object_from_char(0, 0, 'c', 0);

}

void VarTracker::update() {
	std::string collect = "Tracked variables:\n";	// probably faster to just construct a new string  
							// every time, instead of clear()ing a static one
	static const std::string separator = "\n" + std::string(tracker_width_chars - 1, '-') + "\n";
	collect.reserve(TRACKED_MAX*TRACKED_LEN_MAX);
	for (auto &it : tracked) {
		collect += separator + it->name + ":\n" + it->print();
	}
	//collect += separator;
	
	VarTracker::update_VBO(collect);
}

void VarTracker::update_VBO(const std::string &buffer) {
	
	int i = 0;
	
	int x_pos = 0, y_pos = 0;
	int line_beg_index = 0;
	int length = buffer.length();
	int current_line_num = 0;

	for (i = 1; i < length+1; ++i) {

		char c = buffer[i-1];

		if (c == '\n' || i - line_beg_index >= tracker_width_chars) {
			++current_line_num;
			y_pos = current_line_num;
			x_pos = -1;	// workaround, is incremented at the bottom of the lewp
			line_beg_index = i;
		}
	
		VT_char_object_buffer[i] = char_object_from_char(x_pos, y_pos, c, 0);
		++x_pos;
	}		
	
	VarTracker::cur_total_length = buffer.length() + 1;
	glBindBuffer(GL_ARRAY_BUFFER, VarTracker::VT_VBOid);
	glBufferSubData(GL_ARRAY_BUFFER, 0, VarTracker::cur_total_length*(sizeof(char_object)), (const GLvoid*)VT_char_object_buffer);

}

void VarTracker::draw() {
	
	VarTracker::update();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glUseProgram(text_shader->getProgramHandle());
	glBindVertexArray(vartracker_VAOid);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, text_texId);
	text_shader->update_uniform_1i("texture_color", 0);
	
	static const vec4 tracker_overlay_color(0.02, 0.02, 0.02, 0.6);
	static const vec4 tracker_text_color(0.91, 0.91, 0.91, 1.0);
	
	static mat4 tracker_modelview = mat4::translate(vec4(WINDOW_WIDTH-tracker_width_chars*char_spacing_horiz-2, 0, 0, 1.0));

	text_shader->update_uniform_mat4("Projection", text_Projection);
	text_shader->update_uniform_vec4("text_color", tracker_overlay_color);
	text_shader->update_uniform_mat4("ModelView", tracker_modelview);
	
	text_shader->update_uniform_vec4("text_color", tracker_text_color);
	glDrawArrays(GL_POINTS, 0, VarTracker::cur_total_length);
	glBindVertexArray(0);
	glDisable(GL_BLEND);
}

void VarTracker::track(const TrackableBase *const var) {
	auto iter = tracked.begin();

	while (iter != tracked.end()) {
		if ((*iter)->data == var->data) {
			// don't add, we already are tracking this var
			return;
		}
		++iter;
	}
	VarTracker::tracked.push_back(var);
	//PRINT("Tracker: added %s with value %s.\n", var->name.c_str(), var->print().c_str());
}

void VarTracker::untrack(const void *const data_ptr) {
	// search tracked vector for an entity with this data address
	auto iter = tracked.begin();

	while (iter != tracked.end()) {
		if ((*iter)->data == data_ptr) { 
			tracked.erase(iter);
			break;
		}
		++iter;
	}

}

void VarTracker::update_position() {
	VarTracker::pos_x = WINDOW_WIDTH - tracker_width_chars*char_spacing_horiz;
}








#include "text.h"

#include "net/client.h"

extern std::string get_timestamp();

mat4 text_Projection; 
GLuint text_texId;

ShaderProgram *text_shader = NULL;
ShaderProgram *overlay_shader = NULL;

static GLuint text_VAOids[2];
enum { text_VAOid_log = 0, text_VAOid_inputfield = 1 };

static GLuint overlay_VAOid;

struct overlay_rect {
	GLshort UL_corner_pos[2];
	GLshort dim[2];
	overlay_rect() {};
	overlay_rect(GLshort ulc_x, GLshort ulc_y, GLshort dim_x, GLshort dim_y) {
		UL_corner_pos[0] = ulc_x;
		UL_corner_pos[1] = ulc_y;
		dim[0] = dim_x;
		dim[1] = dim_y;
	}
};

static GLuint overlay_VBOid;

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

mat4 onScreenLog::modelview;
GLuint onScreenLog::OSL_VBOid = 0;

int onScreenLog::line_length = OSL_LINE_LEN;
int onScreenLog::num_lines_displayed = 32;
long onScreenLog::current_char_index = 0;
int onScreenLog::current_line_num = 0;
char_object onScreenLog::OSL_char_object_buffer[OSL_BUFFER_SIZE];
bool onScreenLog::visible_ = true;
bool onScreenLog::autoscroll_ = true;
int onScreenLog::num_characters_drawn = 1;	
int onScreenLog::scroll_pos = 0;	
int onScreenLog::current_cobuf_index = 0;


GLint onScreenLog::OSL_upper_left_corner_pos[2] = { 7, WINDOW_HEIGHT - onScreenLog::num_lines_displayed * char_spacing_vert };
GLint VarTracker::VT_upper_left_corner_pos[2] = { WINDOW_WIDTH - TRACKER_LINE_LEN * char_spacing_horiz, 5 };

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


static void setup_overlays() {
	glGenVertexArrays(1, &overlay_VAOid);

	overlay_VBOid = generate_empty_VBO(2*sizeof(overlay_rect), GL_DYNAMIC_DRAW);

	struct overlay_rect rects[2];
	rects[0] = overlay_rect(onScreenLog::OSL_upper_left_corner_pos[0], onScreenLog::OSL_upper_left_corner_pos[1], 
							onScreenLog::get_line_length(), onScreenLog::get_lines_displayed());

	rects[1] = overlay_rect(VarTracker::VT_upper_left_corner_pos[0], VarTracker::VT_upper_left_corner_pos[1],
							TRACKER_LINE_LEN, VarTracker::get_num_tracked());		


	glBindVertexArray(overlay_VAOid);

	glEnableVertexAttribArray(OVERLAY_ATTRIB_POS);
	glBindBuffer(GL_ARRAY_BUFFER, overlay_VBOid);
	glBufferData(GL_ARRAY_BUFFER, 2*sizeof(overlay_rect), rects, GL_DYNAMIC_DRAW);
	glVertexAttribIPointer(OVERLAY_ATTRIB_POS, 4, GL_SHORT, sizeof(overlay_rect), BUFFER_OFFSET(0));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(OVERLAY_ATTRIB_POS);

}

void draw_overlays(const vec4 &color) {
	
	glUseProgram(overlay_shader->getProgramHandle());
	glEnableVertexAttribArray(OVERLAY_ATTRIB_POS);
	
	GLint dummy = 0;
	glBindVertexArray(overlay_VAOid);
	overlay_shader->update_uniform_mat4("Projection", text_Projection);
	overlay_shader->update_uniform_mat4("ModelView", mat4::identity());
	overlay_shader->update_uniform_vec4("overlay_color", color);

	glDrawArrays(GL_POINTS, 0, 2);
	
	glBindVertexArray(0);
}

void update_overlays() {
	
	struct overlay_rect rects[2];
	rects[0] = overlay_rect(onScreenLog::OSL_upper_left_corner_pos[0] - char_spacing_horiz, onScreenLog::OSL_upper_left_corner_pos[1] - char_spacing_vert - 4, 
							onScreenLog::get_line_length() + 1, onScreenLog::get_lines_displayed() + 2);

	rects[1] = overlay_rect(VarTracker::VT_upper_left_corner_pos[0] - char_spacing_horiz, VarTracker::VT_upper_left_corner_pos[1] - char_spacing_vert,
							TRACKER_LINE_LEN + 1, 3 + 3*VarTracker::get_num_tracked() + 1);	
	glBindBuffer(GL_ARRAY_BUFFER, overlay_VBOid);
	glBufferData(GL_ARRAY_BUFFER, 2*sizeof(overlay_rect), rects, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void onScreenLog::draw() {
	
	if (!visible_) { return; }
	
	glUseProgram(text_shader->getProgramHandle());
	glBindVertexArray(text_VAOids[text_VAOid_log]);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, text_texId);
	text_shader->update_uniform_1i("texture_color", 0);
	
	text_shader->update_uniform_mat4("Projection", text_Projection);
	text_shader->update_uniform_mat4("ModelView", onScreenLog::modelview);

	// the scissor area origin is at the "bottom left corner of the screen".
	glEnable(GL_SCISSOR_TEST);
	glScissor(0, (GLint)1.5*char_spacing_vert, (GLsizei) WINDOW_WIDTH, (GLsizei) num_lines_displayed * char_spacing_vert);
	const int OSL_CHARACTERS_PER_VIEW_MAX = OSL_LINE_LEN * num_lines_displayed;

	glDrawArrays(GL_POINTS, 0, MINIMUM(current_char_index, OSL_BUFFER_SIZE));
	glDisable(GL_SCISSOR_TEST);

	glBindVertexArray(0);

	input_field.draw();	// it's only drawn if its enabled

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
	
	onScreenLog::OSL_upper_left_corner_pos[0] = 7;
	onScreenLog::OSL_upper_left_corner_pos[1] = WINDOW_HEIGHT - onScreenLog::num_lines_displayed * char_spacing_vert;

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

	setup_overlays();

	return 1;
}

void onScreenLog::update_position() {
	onScreenLog::OSL_upper_left_corner_pos[0] = 7;
	onScreenLog::OSL_upper_left_corner_pos[1] = WINDOW_HEIGHT - onScreenLog::num_lines_displayed * char_spacing_vert;
}
void onScreenLog::update_modelview() {
	onScreenLog::modelview = mat4::translate(1, onScreenLog::OSL_upper_left_corner_pos[1] - (current_line_num - num_lines_displayed - scroll_pos + 1)*char_spacing_vert, 0.0);
}

void onScreenLog::translate_glyph_buffer(int offset) {
	// a sensible+safe offset arg would be -(32k - OSL_BUFFER_SIZE). 
	for (int i = 0; i < OSL_BUFFER_SIZE; ++i) {
		onScreenLog::OSL_char_object_buffer[i].co_union.bitfields.pos_y += offset;
	}
	current_line_num += offset;
	onScreenLog::update_modelview();
}

void onScreenLog::update_VBO(const std::string &buffer) {
	
	static char_object object_buf[OSL_BUFFER_SIZE];	// enough room to print any string ^_^

	// the proper way to do this would be to precalculate the number of lines occupied by the
	// string to be printed (^ buffer); instead we're using a arbitrary constant (1K)

	const size_t length = buffer.length();

	size_t i = 0;
	static int x_pos = -1, y_pos = 0;
	static int line_beg_index = 0;

	int prev_current_cobuf_index = current_cobuf_index;
	int prev_current_char_index = current_char_index;

	for (i = 0; i < length; ++i) {
		++x_pos;
		char c = buffer[i];

		int char_index = prev_current_char_index + i;

		if (c == '\n' || (char_index-line_beg_index) >= OSL_LINE_LEN) {
			++current_line_num;
			y_pos = current_line_num;
			x_pos = 0;	
			line_beg_index = char_index;
		}
		object_buf[i] = char_object_from_char(x_pos, y_pos, c, 0);
	}
	current_char_index += length;

	int excess = ((current_cobuf_index + length) - OSL_BUFFER_SIZE);
	excess = MAXIMUM(0, excess);
	int fitting = length - excess;
	
	memcpy(&onScreenLog::OSL_char_object_buffer[prev_current_cobuf_index], &object_buf[0], fitting*sizeof(char_object));
	memcpy(&onScreenLog::OSL_char_object_buffer[0], &object_buf[fitting], excess*sizeof(char_object));
	current_cobuf_index = excess > 0 ? excess : current_cobuf_index + fitting;
	
	if (current_line_num >= OSL_BUFFER_SIZE*2) {		
		int amount_to_translate = -(int)OSL_BUFFER_SIZE;
		fprintf(stderr, "current_line_num = %lld, translating by %d\n", (long long)current_line_num, amount_to_translate);
		translate_glyph_buffer(amount_to_translate);	
		// this will work even should the current buffer be composed of 8k newline characters
	} 
	
	glBindBuffer(GL_ARRAY_BUFFER, OSL_VBOid);		
	glBufferSubData(GL_ARRAY_BUFFER, 0, OSL_BUFFER_SIZE*sizeof(char_object), onScreenLog::OSL_char_object_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


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
	update_modelview();
}

void onScreenLog::scroll(int d) {
	scroll_pos += d;
	if (scroll_pos < 0) {
		scroll_pos = 0;
	}
	else if (scroll_pos == 0) {
		autoscroll_ = true;
	} else if (scroll_pos >= (current_line_num - num_lines_displayed)) {
		scroll_pos = current_line_num - num_lines_displayed;
	}
	update_modelview();
}

void onScreenLog::clear() {
	current_char_index = 0;
	current_cobuf_index = 0;
	current_line_num = 0;
	num_characters_drawn = 0;	
	onScreenLog::modelview = mat4::identity();
}

void onScreenLog::InputField::draw() const {
if (!enabled_) { return; }
	
	glUseProgram(text_shader->getProgramHandle());
	glBindVertexArray(text_VAOids[text_VAOid_inputfield]);
	
	//static const vec4 input_field_text_color(_RGB(243, 248, 111), 1.0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, text_texId);
	text_shader->update_uniform_1i("texture_color", 0);
	//text_shader->update_uniform_vec4("text_color", input_field_text_color);

	const mat4 InputField_modelview = mat4::translate(7, WINDOW_HEIGHT-char_spacing_vert, 0.1);

	text_shader->update_uniform_mat4("ModelView", (const GLfloat*)InputField_modelview.rawData());
	text_shader->update_uniform_mat4("Projection", (const GLfloat*)text_Projection.rawData());
	
	glDrawArrays(GL_POINTS, 0, input_buffer.length()+1);	// +1 for the cursor ^^
	glBindVertexArray(0);
}

void onScreenLog::InputField::clear() {
	input_buffer.clear();
	cursor_pos = 0;
	IF_char_object_buffer[0] = char_object_from_char(0, 0, CURSOR_GLYPH, 0);
	update_VBO();
}

void onScreenLog::InputField::enable() {
	if (enabled_ == false) {
		clear();
		refresh();
	}
	enabled_=true;
}

void onScreenLog::InputField::disable() {
	if (enabled_ == true) {
		clear();
	}
	enabled_ = false;
}

void onScreenLog::InputField::submit_and_parse() {
	LocalClient::parse_user_input(input_buffer);
	clear();
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
	changed_ = true;
}

void onScreenLog::InputField::refresh() {
	if (enabled_ && changed_) {
		update_VBO();
		changed_ = false;
	}
}

void onScreenLog::InputField::delete_char_before_cursor_pos() {
	if (cursor_pos < 1) { return; }
	input_buffer.erase(input_buffer.begin() + (cursor_pos-1));
	move_cursor(-1);
	changed_ = true;
}

void onScreenLog::InputField::move_cursor(int amount) {
	cursor_pos += amount;
	cursor_pos = MAXIMUM(cursor_pos, 0);
	cursor_pos = MINIMUM(cursor_pos, (int)input_buffer.length());
	changed_ = true;
}

void onScreenLog::InputField::update_VBO() {

	GLushort x_offset = 0;
	size_t i = 0;
	for (; i < input_buffer.length(); ++i) {
		IF_char_object_buffer[i] = char_object_from_char(x_offset, 0, input_buffer[i], 0);
		++x_offset;
	}

	IF_char_object_buffer[i] = char_object_from_char(x_offset, 0, CURSOR_GLYPH, TEXT_COLOR_YELLOW);
	
	glBindBuffer(GL_ARRAY_BUFFER, onScreenLog::InputField::IF_VBOid);
	glBufferSubData(GL_ARRAY_BUFFER, 0, (input_buffer.length()+1)*sizeof(char_object), (const GLvoid*)&IF_char_object_buffer[0]);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

 // *** STATIC VAR DEFS FOR VARTRACKER ***

GLuint VarTracker::VT_VBOid;
std::vector<const TrackableBase* const> VarTracker::tracked;

int VarTracker::cur_total_length = 0;
char_object VarTracker::VT_char_object_buffer[TRACKED_MAX*TRACKED_LEN_MAX];	
bool VarTracker::has_changed_ = false;

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
	if (has_changed_) {
		update_overlays();
		has_changed_ = false;
	}
	VarTracker::VT_upper_left_corner_pos[0] = WINDOW_WIDTH - TRACKER_LINE_LEN*char_spacing_horiz;
	std::string collect = "Tracked variables:\n";	// probably faster to just construct a new string  
							// every time, instead of clear()ing a static one
	static const std::string separator = "\n" + std::string(TRACKER_LINE_LEN - 1, '-') + "\n";
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

	for (i = 0; i < length; ++i) {

		char c = buffer[i];

		if (c == '\n' || i - line_beg_index >= TRACKER_LINE_LEN) {
			++current_line_num;
			y_pos = current_line_num;
			x_pos = -1;	// workaround, is incremented at the bottom of the lewp
			line_beg_index = i;
		}
	
		VT_char_object_buffer[i] = char_object_from_char(x_pos, y_pos, c, 0);
		++x_pos;
	}		
	
	VarTracker::cur_total_length = buffer.length();
	glBindBuffer(GL_ARRAY_BUFFER, VarTracker::VT_VBOid);
	glBufferSubData(GL_ARRAY_BUFFER, 0, VarTracker::cur_total_length*(sizeof(char_object)), (const GLvoid*)VT_char_object_buffer);

}

void VarTracker::draw() {
	
	VarTracker::update();

	glUseProgram(text_shader->getProgramHandle());
	glBindVertexArray(vartracker_VAOid);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, text_texId);
	text_shader->update_uniform_1i("texture_color", 0);
	
	static const vec4 tracker_overlay_color(0.02, 0.02, 0.02, 0.6);
	static const vec4 tracker_text_color(0.91, 0.91, 0.91, 1.0);
	
	mat4 tracker_modelview = mat4::translate(WINDOW_WIDTH - TRACKER_LINE_LEN*char_spacing_horiz, 5.0, 0.0);

	text_shader->update_uniform_mat4("Projection", text_Projection);
	text_shader->update_uniform_vec4("text_color", tracker_overlay_color);
	text_shader->update_uniform_mat4("ModelView", tracker_modelview);
	
	text_shader->update_uniform_vec4("text_color", tracker_text_color);
	glDrawArrays(GL_POINTS, 0, VarTracker::cur_total_length);
	glBindVertexArray(0);

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
	has_changed_ = true;
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
	VarTracker::VT_upper_left_corner_pos[0] = WINDOW_WIDTH - char_spacing_horiz * TRACKER_LINE_LEN;
	has_changed_ = true;
	update_overlays();
}









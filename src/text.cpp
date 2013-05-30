#include "text.h"

mat4 text_Projection = mat4::proj_ortho(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0, -1.0, 1.0);
mat4 text_ModelView = mat4::identity();
GLuint text_texId;

ShaderProgram *text_shader = NULL;

static GLuint text_shared_IBOid;

static const unsigned shared_indices_count = 0xFFFF - 0xFFFF%6;

PrintQueue onScreenLog::print_queue;
float onScreenLog::pos_x = 2.0, onScreenLog::pos_y = HALF_WINDOW_HEIGHT;
mat4 onScreenLog::modelview = mat4::identity();
GLuint onScreenLog::VBOid = 0;
float char_spacing_vert = 11.0;
float char_spacing_horiz = 7.0;
unsigned onScreenLog::line_length = OSL_LINE_LEN;
unsigned onScreenLog::num_lines_displayed = 30;
unsigned onScreenLog::current_index = 0;
unsigned onScreenLog::current_line_num = 0;
bool onScreenLog::_visible = true;
bool onScreenLog::_autoscroll = true;

void PrintQueue::add(const std::string &s) {
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

static inline GLuint texcoord_index_from_char(char c){ return c == '\0' ? BLANK_GLYPH : (GLuint)c - 0x20; }

static const struct xy glyph_base[4] = { 
		{0.0, 0.0}, {0.0, 12.0}, {6.0, 12.0}, {6.0, 0.0} 
};

static struct xy get_glyph_xy(int index, float x, float y) {
	struct xy _xy = { (x) + glyph_base[(index)].x, (y) + glyph_base[(index)].y };
	return _xy;
}

static inline glyph glyph_from_char(float x, float y, char c) { 
	glyph g;

	const unsigned tindex = texcoord_index_from_char(c);

	for (unsigned i = 0; i < 4; ++i) {
		g.vertices[i] = vertex2(get_glyph_xy(i, x, y), glyph_texcoords[tindex][i]);
	}
	return g;

}

static GLushort *generateSharedTextIndices() {
	
	GLushort *indices = new GLushort[shared_indices_count];

	unsigned int i = 0;
	unsigned int j = 0;
	while (i < shared_indices_count) {

		indices[i] = j;
		indices[i+1] = j+1;
		indices[i+2] = j+3;
		indices[i+3] = j+1;
		indices[i+4] = j+2;
		indices[i+5] = j+3;

		i += 6;
		j += 4;
	}
	return indices;

}

void text_generate_shared_IBO() {
	GLushort *indices = generateSharedTextIndices();
	glGenBuffers(1, &text_shared_IBOid);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_shared_IBOid);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, shared_indices_count * sizeof(GLushort), (const GLvoid*)indices, GL_STATIC_DRAW);

	delete [] indices;
}



int onScreenLog::init() {
	generate_VBO();
	text_generate_shared_IBO();
	return 1;
}



void onScreenLog::update_VBO(const char* buffer, unsigned length) {
	
	glyph *glyphs = new glyph[length];	

	int i = 0;
	static float x_adjustment = 0, y_adjustment = 0;
	static int line_beg_index = 0;

	for (i = 0; i < length; ++i) {

		char c = buffer[i];

		if (c == '\n') {
			++current_line_num;
			y_adjustment = current_line_num*char_spacing_vert;
			x_adjustment = -char_spacing_horiz;	// workaround, is incremented at the bottom of the lewp
			line_beg_index = (current_index+i);
		}

		else if (((current_index+i)-line_beg_index) >= OSL_LINE_LEN) {
			++current_line_num;
			y_adjustment = current_line_num*char_spacing_vert;
			x_adjustment = 0;
			line_beg_index = (current_index+i);
		}
	
		glyphs[i] = glyph_from_char(pos_x + x_adjustment, pos_y + y_adjustment, c);
		
		x_adjustment += char_spacing_horiz;
	}
	
	unsigned prev_current_index = current_index;
	current_index += length;
	
	glBindBuffer(GL_ARRAY_BUFFER, VBOid);
	if (current_index > OSL_BUFFER_SIZE) {

		// the excessive part will be flushed to the beginning of the VBO :P
		unsigned excess = current_index - OSL_BUFFER_SIZE + 1;
		unsigned fitting = length - excess;
		fprintf(stderr, 
		"onScreenLog: current_index > OSL_BUFFER_SIZE (%u > %u).\n length = %u. calling glBufferSubdata with fitting = %u, excess = %u\n", 
		current_index, OSL_BUFFER_SIZE, length, fitting, excess);
		glBufferSubData(GL_ARRAY_BUFFER, prev_current_index*sizeof(glyph), fitting*sizeof(glyph), (const GLvoid*)glyphs);
		glBufferSubData(GL_ARRAY_BUFFER, 0*sizeof(glyph), excess*sizeof(glyph), (const GLvoid*)(glyphs + fitting));
		current_index = excess;
	}
	else {
		glBufferSubData(GL_ARRAY_BUFFER, prev_current_index*sizeof(glyph), length*(sizeof(glyph)), (const GLvoid*)glyphs);
	}

	if (_autoscroll) {
		float d = (y_adjustment + pos_y + char_spacing_vert) - WINDOW_HEIGHT;
		if (d > modelview(3,1)) { 
			set_y_translation(-d);
		}
	}
	delete [] glyphs;

}

void onScreenLog::print(const char* fmt, ...) {
	char buffer[OSL_BUFFER_SIZE];
	va_list args;
	va_start(args, fmt);
	SYSTEMTIME st;
    GetSystemTime(&st);
	std::size_t timestamp_len = 0; //sprintf(buffer, "%02d:%02d:%02d.%03d > ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	std::size_t msg_len = vsprintf_s(buffer + timestamp_len, OSL_BUFFER_SIZE, fmt, args);
	std::size_t total_len = timestamp_len + msg_len;
	buffer[total_len] = '\0';
	va_end(args);

	print_queue.add(buffer);

}

void onScreenLog::print_string(const std::string &s) {
	update_VBO(s.c_str(), s.length());
}

void onScreenLog::scroll(float ds) {
	float bottom_scroll_displacement = (current_line_num + 1)*char_spacing_vert + pos_y - WINDOW_HEIGHT;
	
	modelview(3, 1) += ds;
	
	if (bottom_scroll_displacement + modelview(3,1) >= 0) {
		_autoscroll = false;

	}
	else {		
		set_y_translation(-bottom_scroll_displacement);
		_autoscroll = true;
	}

}

void onScreenLog::generate_VBO() {
	glGenBuffers(1, &VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, VBOid);
	glBufferData(GL_ARRAY_BUFFER, OSL_BUFFER_SIZE*sizeof(glyph), NULL, GL_DYNAMIC_DRAW);

	// actual vertex data will be sent by subsequent glBufferSubdata calls
}

void onScreenLog::set_y_translation(float new_y) {
	modelview(3,1) = new_y;
}

void onScreenLog::draw() {
	if (!_visible) { return; }
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glScissor(0, 0, WINDOW_WIDTH, num_lines_displayed * char_spacing_vert);
	glBindBuffer(GL_ARRAY_BUFFER, VBOid);
	
	glVertexAttribPointer(ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(0));
	glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(2*sizeof(float)));
	glUseProgram(text_shader->getProgramHandle());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, text_texId);
	text_shader->update_uniform_1i("texture1", 0);

	text_shader->update_uniform_mat4("ModelView", (const GLfloat*)modelview.rawData());
	text_shader->update_uniform_mat4("Projection", (const GLfloat*)text_Projection.rawData());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_shared_IBOid);	// uses a shared index buffer.
	
	glDrawElements(GL_TRIANGLES, 6*OSL_BUFFER_SIZE, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
	glDisable(GL_SCISSOR_TEST);
}	

void onScreenLog::clear() {
	current_index = 0;
	current_line_num = 0;
	modelview = mat4::identity();
}






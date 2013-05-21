#include "text.h"

mat4 text_Projection = mat4::proj_ortho(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0, -1.0, 1.0);
mat4 text_ModelView = mat4::identity();
GLuint text_texId;

ShaderProgram *text_shader = NULL;

static const float char_spacing_horiz = 7.0;
static const float char_spacing_vert = 11.0;

static const unsigned common_indices_count = 0xFFFF - 0xFFFF%6;

GLuint wpstring_holder::static_VBOid = 0;
GLuint wpstring_holder::dynamic_VBOid = 0;
GLuint wpstring_holder::shared_IBOid = 0;

std::size_t wpstring_holder::static_strings_total_length = 0;

std::vector<wpstring> wpstring_holder::static_strings;
std::vector<wpstring> wpstring_holder::dynamic_strings;

static inline GLuint texcoord_index_from_char(char c){ return c == '\0' ? BLANK_GLYPH : (GLuint)c - 0x20; }

static inline glyph glyph_from_char(float x, float y, char c) { 
	glyph g;
	
	int j = 0;
	const GLuint tindex = texcoord_index_from_char(c);

	// behold the mighty unrolled loop

	g.vertices[0] = vertex2(x + ((j>>1)&1)*6.0, 
			  	y + (((j+1)>>1)&1)*12.0,
			  	glyph_texcoords[tindex][2*j], 
			  	glyph_texcoords[tindex][2*j+1]);
	

	j = 1;
	g.vertices[1] = vertex2(x + ((j>>1)&1)*6.0, 
			  	y + (((j+1)>>1)&1)*12.0,
			  	glyph_texcoords[tindex][2*j], 
			  	glyph_texcoords[tindex][2*j+1]);

	j = 2;
	g.vertices[2] = vertex2(x + ((j>>1)&1)*6.0, 
			  	y + (((j+1)>>1)&1)*12.0,
			  	glyph_texcoords[tindex][2*j], 
			  	glyph_texcoords[tindex][2*j+1]);


	j = 3;
	g.vertices[3] = vertex2(x + ((j>>1)&1)*6.0, 
				y + (((j+1)>>1)&1)*12.0,
			  	glyph_texcoords[tindex][2*j], 
				glyph_texcoords[tindex][2*j+1]);

	return g;

}



wpstring::wpstring(const std::string &text_, GLuint x_, GLuint y_) : x(x_), y(y_) {

	actual_size = text_.length();
	fprintf(stderr, "wpstring: constructing object with actual size %d.\n", actual_size);
	text.fill(0x20);
	
	if (actual_size > WPSTRING_LENGTH_MAX) {
		onScreenLog::print( "text: warning: string length exceeds %d, truncating.\nstring: \"%s\"\n", WPSTRING_LENGTH_MAX, text_.c_str());
		std::string sub = text_.substr(0, WPSTRING_LENGTH_MAX);
		std::copy(sub.begin(), sub.end(), text.data());

	}
	else {
		const std::size_t diff = WPSTRING_LENGTH_MAX - actual_size;
		std::copy(text_.begin(), text_.end(), text.data()); 
	}

	visible = true;
}


void wpstring::updateString(const std::string &newtext, int offset, int index) {
	
	//offset = 0;
	actual_size = newtext.length() + offset;

	//y += char_spacing_vert;

	if (actual_size > WPSTRING_LENGTH_MAX) {	
		std::string sub = newtext.substr(0, WPSTRING_LENGTH_MAX);
		std::copy(sub.begin(), sub.end(), text.data());
		actual_size = WPSTRING_LENGTH_MAX;
	}
	else {
		const std::size_t diff = WPSTRING_LENGTH_MAX - actual_size;
		std::copy(newtext.begin(), newtext.end(), text.data()); 
	}
	
	glyph glyphs[WPSTRING_LENGTH_MAX];

	float x_adjustment = 0;
	float y_adjustment = 0;
	
	int i = 0, j = 0;

	for (i = 0; i < WPSTRING_LENGTH_MAX; ++i) {
		if ((x + x_adjustment) > (WINDOW_WIDTH - char_spacing_horiz) || text[i] == '\n') {
				y_adjustment += char_spacing_vert;
				x_adjustment = (text[i] == '\n') ? -char_spacing_horiz : 0;	// ugly workaround. without this the newline characters will also occupy horizontal space
		}
	
		glyphs[i] = glyph_from_char(x+x_adjustment, y + y_adjustment, text[i]);
		x_adjustment += char_spacing_horiz;
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, wpstring_holder::get_dynamic_VBOid());
	glBufferSubData(GL_ARRAY_BUFFER, (index*WPSTRING_LENGTH_MAX + offset)*sizeof(glyph), (WPSTRING_LENGTH_MAX-offset)*sizeof(glyph), (const GLvoid*)glyphs);

}


GLushort *generateCommonTextIndices() {
	
	GLushort *indices = new GLushort[common_indices_count];

	unsigned int i = 0;
	unsigned int j = 0;
	while (i < common_indices_count) {

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

void wpstring_holder::updateDynamicString(int index, const std::string& newtext, int offset) {
	dynamic_strings[index].updateString(newtext, offset, index);
}


void wpstring_holder::append(const wpstring& str, GLuint static_mask) {
	if (static_mask) {
		static_strings.push_back(str);
		static_strings_total_length += str.getActualSize();
	}
	else {
		dynamic_strings.push_back(str);
	}
}


void wpstring_holder::createBufferObjects() {

	onScreenLog::print( "sizeof(glyph_texcoords): %lu\n", sizeof(glyph_texcoords)/(8*sizeof(float)));

	GLushort *common_text_indices = generateCommonTextIndices();

	glGenBuffers(1, &static_VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, static_VBOid);

	glyph *glyphs = new glyph[static_strings_total_length];

	unsigned int i = 0,j = 0, g = 0;
	
	float x_adjustment = 0.0;
	float y_adjustment = 0.0;
	std::vector<wpstring>::iterator static_iter = static_strings.begin();
	
	while(static_iter != static_strings.end()) {
		x_adjustment = 0;
		y_adjustment = 0;
		
		std::size_t current_string_length = static_iter->getActualSize();
		for (i = 0; i < current_string_length; i++) {
			if ((static_iter->x + x_adjustment) > (WINDOW_WIDTH - 7.0)) {
				y_adjustment += char_spacing_vert;
				x_adjustment = 0;
			}
			glyphs[g] = glyph_from_char(static_iter->x + x_adjustment, static_iter->y + y_adjustment, static_iter->text[i]);
			++g;
			x_adjustment += char_spacing_horiz;
		}
		++static_iter;
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(glyph) * static_strings_total_length, (const GLvoid*)glyphs, GL_STATIC_DRAW); 

	delete [] glyphs;

	glGenBuffers(1, &dynamic_VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, dynamic_VBOid);

	glyphs = new glyph[WPSTRING_LENGTH_MAX*dynamic_strings.size()];
	
	i = 0,j = 0, g = 0;

	std::vector<wpstring>::iterator dynamic_iter;
	dynamic_iter = dynamic_strings.begin();

	glyphs[g] = glyph_from_char(dynamic_iter->x + x_adjustment, dynamic_iter->y, dynamic_iter->text[i]);
	while(dynamic_iter != dynamic_strings.end()) {
		x_adjustment = 0;
		y_adjustment = 0;
		for (i=0; i < WPSTRING_LENGTH_MAX; i++) {
			
			if ((dynamic_iter->x + x_adjustment) > (WINDOW_WIDTH - 7.0)) {
				y_adjustment += char_spacing_vert;
				x_adjustment = 0;
			}
			glyphs[g] = glyph_from_char(dynamic_iter->x + x_adjustment, dynamic_iter->y + y_adjustment, dynamic_iter->text[i]);
			++g;
			x_adjustment += char_spacing_horiz;
		}
		++dynamic_iter;
	}
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(glyph) * dynamic_strings.size()*WPSTRING_LENGTH_MAX, (const GLvoid*)glyphs, GL_DYNAMIC_DRAW);

	delete [] glyphs;


	glGenBuffers(1, &shared_IBOid);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shared_IBOid);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (common_indices_count-1)*sizeof(GLushort), (const GLvoid*)common_text_indices, GL_STATIC_DRAW);

	delete [] common_text_indices;

}

const wpstring &wpstring_holder::getDynamicString(int index) {
	
	if (index >= dynamic_strings.size()) {
		fprintf(stderr, "Warning: getDynamicString requested with index >= dynamic_strings.size(). Returning string from index 0.\n");
		index = 0;
	}
	return dynamic_strings[index];

}

float onScreenLog::pos_x = 2.0, onScreenLog::pos_y = HALF_WINDOW_HEIGHT;
mat4 onScreenLog::modelview = mat4::identity();
std::array<char, ON_SCREEN_LOG_BUFFER_SIZE> onScreenLog::data;
GLuint onScreenLog::VBOid = 0;
unsigned onScreenLog::most_recent_index = 0;
unsigned onScreenLog::most_recent_line_num = 0;
std::deque<unsigned char> onScreenLog::line_lengths;

int onScreenLog::init() {
	data.fill(0x20);
	generate_VBO();
	return 1;
}

void onScreenLog::update_VBO(const char* buffer, unsigned length) {
	
	// maximum possible amount of whitespace padding is ON_SCREEN_LOG_LINE_LEN - 1.
	glyph *glyphs = new glyph[length];	

	unsigned i = 0;
	static float x_adjustment = 0, y_adjustment = 0;
	static unsigned line_beg_index = 0;

	for (i = 0; i < length; ++i) {

		char c = buffer[i];

		if (c == '\n') {
			y_adjustment += char_spacing_vert;
			x_adjustment = -char_spacing_horiz;	// workaround, is incremented at the bottom of the lewp
			line_lengths.push_back((unsigned char)(i - line_beg_index));
			if (line_lengths.size() > ON_SCREEN_LOG_NUM_LINES) {
				line_lengths.pop_front();
			}
			line_beg_index = i;
			++most_recent_line_num;
		}

		else if (i % ON_SCREEN_LOG_LINE_LEN == ON_SCREEN_LOG_LINE_LEN_MINUS_ONE) {
			y_adjustment += char_spacing_vert;
			x_adjustment = 0;
			line_lengths.push_back((unsigned char)ON_SCREEN_LOG_LINE_LEN);
			if (line_lengths.size() > ON_SCREEN_LOG_NUM_LINES) {
				line_lengths.pop_front();
			}
			line_beg_index = i;
			++most_recent_line_num;
		}
	
		glyphs[i] = glyph_from_char(pos_x + x_adjustment, pos_y + y_adjustment, c);
		
		//fprintf(stderr, "glyph: %f %f %f %f\n",glyphs[i].vertices[0].vx, glyphs[i].vertices[0].vy, glyphs[i].vertices[0].u, glyphs[i].vertices[0].v);
		x_adjustment += char_spacing_horiz;
	}
	
	unsigned prev_most_recent_index = most_recent_index;

	glBindBuffer(GL_ARRAY_BUFFER, VBOid);
	glBufferSubData(GL_ARRAY_BUFFER, prev_most_recent_index*sizeof(glyph), length*sizeof(glyph), (const GLvoid*)glyphs);
	
	most_recent_index += length;

	if (0/*SOMETHING!!!*/ > WINDOW_HEIGHT) { scroll(-char_spacing_vert); }

	delete [] glyphs;

}

void onScreenLog::print(const char* fmt, ...) {
	static char buffer[ON_SCREEN_LOG_BUFFER_SIZE];
	va_list args;
	va_start(args, fmt);
	SYSTEMTIME st;
    GetSystemTime(&st);
	std::size_t timestamp_len = 0; //sprintf(buffer, "%02d:%02d:%02d.%03d > ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	std::size_t msg_len = vsprintf(buffer + timestamp_len, fmt, args);
	std::size_t total_len = timestamp_len + msg_len;
	buffer[total_len] = '\0';
	va_end(args);

//	fprintf(stderr, "%s\n", buffer);

	if (most_recent_index + total_len < ON_SCREEN_LOG_BUFFER_SIZE) {	
		update_VBO(buffer, total_len);
	}
	// else fail silently :p
}

void onScreenLog::scroll(float ds) {
	modelview(3, 1) += ds;
}

void onScreenLog::generate_VBO() {
	glGenBuffers(1, &VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, VBOid);
	glBufferData(GL_ARRAY_BUFFER, ON_SCREEN_LOG_BUFFER_SIZE*sizeof(glyph), NULL, GL_DYNAMIC_DRAW);

	// actual vertex data will be sent by subsequent glBufferSubdata calls
}

void onScreenLog::draw() {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);

	glBindBuffer(GL_ARRAY_BUFFER, VBOid);
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(0));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(2*sizeof(float)));
	glUseProgram(text_shader->getProgramHandle());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, text_texId);
	text_shader->update_uniform_1i("texture1", 0);

	text_shader->update_uniform_mat4("ModelView", (const GLfloat*)modelview.rawData());
	text_shader->update_uniform_mat4("Projection", (const GLfloat*)text_Projection.rawData());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wpstring_holder::get_IBOid());	// uses a shared index buffer.
	
	// ranged drawing will be implemented later
	glDrawElements(GL_TRIANGLES, 6*most_recent_index, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));

}	


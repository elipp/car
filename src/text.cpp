#include "text.h"

mat4 text_Projection = mat4::proj_ortho(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0, -1.0, 1.0);
mat4 text_ModelView = mat4::identity();
GLuint text_texId;

ShaderProgram *text_shader = NULL;

static GLuint text_shared_IBOid;

static const unsigned shared_indices_count = 0xFFFF - 0xFFFF%6;

float onScreenLog::pos_x = 2.0, onScreenLog::pos_y = HALF_WINDOW_HEIGHT;
mat4 onScreenLog::modelview = mat4::identity();
GLuint onScreenLog::VBOid = 0;
float char_spacing_vert = 11.0;
float char_spacing_horiz = 7.0;
unsigned onScreenLog::line_length = OSL_LINE_LEN;
unsigned onScreenLog::num_lines_displayed = OSL_BUFFER_SIZE / OSL_LINE_LEN;
unsigned onScreenLog::current_index = 0;
unsigned onScreenLog::current_line_num = 0;
bool onScreenLog::_visible = true;

template<class T>
std::string to_string(T const& t) {
	std::ostringstream os;
	os << t;
	return os.str();
}

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

	unsigned i = 0;
	static float x_adjustment = 0, y_adjustment = 0;
	static unsigned line_beg_index = 0;

	for (i = 0; i < length; ++i) {

		char c = buffer[i];

		if (c == '\n') {
			y_adjustment += char_spacing_vert;
			x_adjustment = -char_spacing_horiz;	// workaround, is incremented at the bottom of the lewp
			
			line_beg_index = i;
			++current_line_num;
		}

		else if (i % OSL_LINE_LEN == OSL_LINE_LEN_MINUS_ONE) {
			y_adjustment += char_spacing_vert;
			x_adjustment = 0;
			line_beg_index = i;
			++current_line_num;
		}
	
		glyphs[i] = glyph_from_char(pos_x + x_adjustment, pos_y + y_adjustment, c);
		
		//fprintf(stderr, "glyph: %f %f %f %f\n",glyphs[i].vertices[0].vx, glyphs[i].vertices[0].vy, glyphs[i].vertices[0].u, glyphs[i].vertices[0].v);
		x_adjustment += char_spacing_horiz;
	}
	
	unsigned prev_current_index = current_index;

	glBindBuffer(GL_ARRAY_BUFFER, VBOid);
	glBufferSubData(GL_ARRAY_BUFFER, prev_current_index*sizeof(glyph), length*sizeof(glyph), (const GLvoid*)glyphs);
	
	current_index += length;

	if ((y_adjustment+pos_y) - modelview(3,1) > WINDOW_HEIGHT) {}
	fprintf(stderr, "(y_adj + pos_y) - modelview(3,1)= %f\n", y_adjustment + pos_y - modelview(3,1));

	delete [] glyphs;

}

void onScreenLog::print(const char* fmt, ...) {
	static char buffer[OSL_BUFFER_SIZE];
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

	if (current_index + total_len < OSL_BUFFER_SIZE) {	
		update_VBO(buffer, total_len);
	}
	// else fail silently X:D:D:Dd a nice, fast solution would be to 
	// just start filling at the beginning of the VBO and use glScissor :P
}

void onScreenLog::scroll(float ds) {
	modelview(3, 1) += ds;
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

	glBindBuffer(GL_ARRAY_BUFFER, VBOid);
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(0));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(2*sizeof(float)));
	glUseProgram(text_shader->getProgramHandle());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, text_texId);
	text_shader->update_uniform_1i("texture1", 0);

	text_shader->update_uniform_mat4("ModelView", (const GLfloat*)modelview.rawData());
	text_shader->update_uniform_mat4("Projection", (const GLfloat*)text_Projection.rawData());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_shared_IBOid);	// uses a shared index buffer.
	
	// ranged drawing will be implemented later
	glDrawElements(GL_TRIANGLES, 6*current_index, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));

}	

void onScreenLog::clear() {
	current_index = 0;
	current_line_num = 0;
	modelview = mat4::identity();
}






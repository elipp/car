#include "text.h"


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
	
	if (actual_size > WPSTRING_LENGTH_MAX) {
		logWindowOutput( "text: warning: string length exceeds %d, truncating.\nstring: \"%s\"\n", WPSTRING_LENGTH_MAX, text_.c_str());
		text = text_.substr(0, WPSTRING_LENGTH_MAX);

	}
	else {
		const std::size_t diff = WPSTRING_LENGTH_MAX - actual_size;
		text = text_;
		text = std::string(WPSTRING_LENGTH_MAX, 0x20);
		text.replace(0, actual_size, text_); 
	}

	visible = true;
}


void wpstring::updateString(const std::string &newtext, int offset, int index) {
	
	actual_size = newtext.length() + offset;

	if (actual_size > WPSTRING_LENGTH_MAX) {	
		text = newtext.substr(0, WPSTRING_LENGTH_MAX);
		actual_size = WPSTRING_LENGTH_MAX;
	}
	else {
		const std::size_t diff = WPSTRING_LENGTH_MAX - actual_size;
		text.replace(offset, actual_size, newtext);
	}
	
	glyph glyphs[WPSTRING_LENGTH_MAX];

	float x_adjustment = 0;
	float y_adjustment = 0;
	
	int i = 0, j = 0;

	for (i = 0; i < WPSTRING_LENGTH_MAX; ++i) {
		if ((x + x_adjustment) > (WINDOW_WIDTH - 7.0) || (text[i] == '\n')) {
				y_adjustment += char_spacing_vert;
				x_adjustment = 0;
		}
	
		glyphs[i] = glyph_from_char(x+x_adjustment, y + y_adjustment, text[i]);
		x_adjustment += char_spacing_horiz;
	}
	
	fprintf(stderr, "Calling buffersubdata with index + 1 = %d, offset = %d\n", index + 1, offset);

	glBindBuffer(GL_ARRAY_BUFFER, wpstring_holder::get_dynamic_VBOid());
	glBufferSubData(GL_ARRAY_BUFFER, ((index+1)*WPSTRING_LENGTH_MAX + offset)*sizeof(glyph), (WPSTRING_LENGTH_MAX-offset)*sizeof(glyph), (const GLvoid*)glyphs);

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

	logWindowOutput( "sizeof(glyph_texcoords): %lu\n", sizeof(glyph_texcoords)/(8*sizeof(float)));

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


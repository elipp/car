#include "text.h"

#define BLANK_GLYPH (sizeof(glyph_texcoords)/(8*sizeof(float)) - 1)

static inline GLuint texcoord_index_from_char(char c){ return c == '\0' ? BLANK_GLYPH : (GLuint)c - 0x20; }

static inline glyph glyph_from_char(float x, float y, char c) { 
	glyph g;
	
	int j = 0;
	const GLuint tindex = texcoord_index_from_char(c);

/*	g.vertices[0].pos = vec2(x + ((j>>1)&1)*6.0, y + (((j+1)>>1)&1)*12.0);
	g.vertices[0].texc = vec2(glyph_texcoords[tindex][2*j], glyph_texcoords[tindex][2*j+1]); */
	
	g.vertices[0] = vertex2(x + ((j>>1)&1)*6.0, 
			  	y + (((j+1)>>1)&1)*12.0,
			  	glyph_texcoords[tindex][2*j], 
			  	glyph_texcoords[tindex][2*j+1]);
	
/*	g.vertices[1].pos = vec2(x + ((j>>1)&1)*6.0, y + (((j+1)>>1)&1)*12.0);
	g.vertices[1].texc = vec2(glyph_texcoords[tindex][2*j], glyph_texcoords[tindex][2*j+1]); */


	j = 1;
	g.vertices[1] = vertex2(x + ((j>>1)&1)*6.0, 
			  	y + (((j+1)>>1)&1)*12.0,
			  	glyph_texcoords[tindex][2*j], 
			  	glyph_texcoords[tindex][2*j+1]);
/*
	g.vertices[2].pos = vec2(x + ((j>>1)&1)*6.0, y + (((j+1)>>1)&1)*12.0);
	g.vertices[2].texc = vec2(glyph_texcoords[tindex][2*j], glyph_texcoords[tindex][2*j+1]); */


	j = 2;
	g.vertices[2] = vertex2(x + ((j>>1)&1)*6.0, 
			  	y + (((j+1)>>1)&1)*12.0,
			  	glyph_texcoords[tindex][2*j], 
			  	glyph_texcoords[tindex][2*j+1]);

/*	g.vertices[3].pos = vec2(x + ((j>>1)&1)*6.0, y + (((j+1)>>1)&1)*12.0);
	g.vertices[3].texc = vec2(glyph_texcoords[tindex][2*j], glyph_texcoords[tindex][2*j+1]); */

	j = 3;
	g.vertices[3] = vertex2(x + ((j>>1)&1)*6.0, 
				y + (((j+1)>>1)&1)*12.0,
			  	glyph_texcoords[tindex][2*j], 
				glyph_texcoords[tindex][2*j+1]);

	return g;

}

static const std::size_t common_indices_count = ((0x1<<16)/6)*6;

GLuint wpstring_holder::static_VBOid = 0;
GLuint wpstring_holder::dynamic_VBOid = 0;
GLuint wpstring_holder::shared_IBOid = 0;

std::size_t wpstring_holder::static_strings_total_length = 0;

std::vector<wpstring> wpstring_holder::static_strings;
std::vector<wpstring> wpstring_holder::dynamic_strings;

wpstring::wpstring(const std::string &text_, GLuint x_, GLuint y_) : x(x_), y(y_) {

	const std::size_t arg_str_len = text_.length();
	
	if (arg_str_len > wpstring_max_length) {
		logWindowOutput( "text: warning: string length exceeds %d, truncating.\nstring: \"%s\"\n", wpstring_max_length, text_.c_str());
		text_.copy(text, wpstring_max_length, 0);
		text[wpstring_max_length-1] = '\0';
		actual_size = wpstring_max_length;

	}
	else {
		const std::size_t diff = wpstring_max_length - arg_str_len;
		text_.copy(text, arg_str_len, 0);
		memset(text+arg_str_len, 0x20, diff);		// likely to segfault :P
		text[wpstring_max_length-1] = '\0';	// not really needed though
		actual_size = arg_str_len;
	}

	visible = true;
}




void wpstring::updateString(const std::string &newtext, int index) {
	
	const std::size_t arg_str_len = newtext.length();
	if (arg_str_len > wpstring_max_length) {	
		const std::string newsub = newtext.substr(0, wpstring_max_length);
		newsub.copy(text, wpstring_max_length, 0);
	}
	else {
		const std::size_t diff = wpstring_max_length - arg_str_len;
		newtext.copy(text, arg_str_len, 0);
		memset(text+arg_str_len, 0x20, diff);	// segfault :D
	}
	
	glyph glyphs[wpstring_max_length];

	float a;

	int i=0, j=0;

	for (i=0; i < wpstring_max_length; i++) {
		a = i * 7.0;	// the distance between two consecutive letters.
		glyphs[i] = glyph_from_char(x+a, y, text[i]);
	}


	glBindBuffer(GL_ARRAY_BUFFER, wpstring_holder::get_dynamic_VBOid());
	glBufferSubData(GL_ARRAY_BUFFER, index*wpstring_max_length*sizeof(glyph), wpstring_max_length*sizeof(glyph), (const GLvoid*)glyphs);

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

void wpstring_holder::updateDynamicString(int index, const std::string& newtext) {
	dynamic_strings[index].updateString(newtext, index);
}


void wpstring_holder::append(const wpstring& str, GLuint static_mask) {
	if (static_mask) {
		static_strings.push_back(str);
		static_strings_total_length += str.getRealSize();
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

	float a;
	
	unsigned int i = 0,j = 0, g = 0;
	
	std::vector<wpstring>::iterator static_iter = static_strings.begin();
	
	while(static_iter != static_strings.end()) {
		std::size_t current_string_length = static_iter->getRealSize();
		for (i=0; i < current_string_length; i++) {
			a = i * 7.0;	// the distance between two consecutive letters.
			glyphs[g] = glyph_from_char(static_iter->x + a, static_iter->y, static_iter->text[i]);
			++g;
		}
		++static_iter;
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(glyph) * static_strings_total_length, (const GLvoid*)glyphs, GL_STATIC_DRAW); 

	delete [] glyphs;

	glGenBuffers(1, &dynamic_VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, dynamic_VBOid);

	glyphs = new glyph[wpstring_max_length*dynamic_strings.size()];
	
	i = 0,j = 0, g = 0;

	std::vector<wpstring>::iterator dynamic_iter;
	dynamic_iter = dynamic_strings.begin();
	glyphs[g] = glyph_from_char(dynamic_iter->x + a, dynamic_iter->y, dynamic_iter->text[i]);
	while(dynamic_iter != dynamic_strings.end()) {

		for (i=0; i < wpstring_max_length; i++) {
			a = i * 7.0;	// the distance between two consecutive letters	is 7 pixels	
			glyphs[g] = glyph_from_char(dynamic_iter->x + a, dynamic_iter->y, dynamic_iter->text[i]);
			++g;
		}
		++dynamic_iter;
	}
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(glyph) * dynamic_strings.size()*wpstring_max_length, (const GLvoid*)glyphs, GL_DYNAMIC_DRAW);

	delete [] glyphs;


	glGenBuffers(1, &shared_IBOid);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shared_IBOid);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (common_indices_count-1)*sizeof(GLushort), (const GLvoid*)common_text_indices, GL_STATIC_DRAW);


	delete [] common_text_indices;

}

std::string wpstring_holder::getDynamicString(int index) {
	
	return std::string(dynamic_strings[index].text);

}


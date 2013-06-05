#ifndef TEXTURE_H
#define TEXTURE_H

#include "glwindow_win32.h"

#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include "common.h"



class Texture {

	private:
		std::string name;
		GLuint textureId;
		unsigned short width;
		unsigned short height;
		bool hasAlpha;

		bool _nosuch;
		bool _badheader;
		bool _otherbad;


	public:
		
		std::string getName() const { return name; }
		GLuint id() const { return textureId; }
		unsigned short getWidth() const { return width; }
		unsigned short getHeight() const { return height; }
		bool getAlpha() const { return hasAlpha; }
		
		bool bad() const { return _nosuch || _badheader || _otherbad; }
		bool nosuch() const { return _nosuch; }
		bool badheader() const { return _badheader; }
		bool otherbad() const { return _otherbad; }

		GLuint getId() const { return textureId; }
		Texture(const std::string &filename, const GLint filter_param); 
	
};


class TextureBank {
	static std::vector<Texture> textures;
public:
	static GLint get_id_by_name(const std::string &name);
	static GLuint add(const Texture &t);
	static size_t get_size() { return textures.size(); }
	static bool validate();
};

typedef struct BMPHEADER {

	// the first two bytes of a bmp file contain the letters "B" and "M".
	// Adding a "signature" element of size 2 to the beginning of this structure
	// would force us to use a struct packing pragma directive
	unsigned int filesize;
	unsigned int dummy0;		// two reserved 2-byte fields, must be zero
	unsigned int dataoffset;
	unsigned int headersize;
	unsigned int width;
	unsigned int height;
	unsigned short dummy1;
	unsigned short bpp;		
	unsigned int compression;	
	unsigned int size;			
	int hres;				
	int vres;					
	unsigned int colors;	
	unsigned int impcolors;	

	BMPHEADER() {
		memset(this, 0, sizeof(BMPHEADER));
	}
} BMPHEADER;


#endif

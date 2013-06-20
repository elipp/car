#ifndef TEXTURE_H
#define TEXTURE_H

#include "glwindow_win32.h"

#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include "common.h"

#define IS_POWER_OF_TWO(x) ((x) & ((x) - 1))

struct unified_header_data {
	unsigned width, height;
	int bpp;
};

class Texture {
	private:
		std::string name;
		GLuint textureId;
		unified_header_data img_info;

		bool _nosuch;
		bool _badheader;
		bool _otherbad;

	public:	
		std::string getName() const { return name; }
		GLuint id() const { return textureId; }
		
		bool bad() const { return _nosuch || _badheader || _otherbad; }
		bool nosuch() const { return _nosuch; }
		bool badheader() const { return _badheader; }
		bool otherbad() const { return _otherbad; }

		GLuint getId() const { return textureId; }
		Texture(const std::string &filename, const GLint filter_param); 
	
};

// this is pretty gay though :DDDd
class TextureBank {
	static std::vector<Texture> textures;
public:
	static GLint get_id_by_name(const std::string &name);
	static GLuint add(const Texture &t);
	static size_t get_size() { return textures.size(); }
	static bool validate();
};

#define LIMIT_VALUE_BETWEEN(VAL, MIN, MAX) max((MIN), min((VAL), (MAX)))

class HeightMap {
	int img_dim_pixels;	// pixels
	int bitdepth;
	std::vector<unsigned char> pixels;
	bool _bad;

	const float real_map_dim;
	const float scale; // units
	const float top;	// maximum elevation
	const float bottom; // minimum elevation
	double dim_per_scale;
	double half_real_map_dim;
	int dim_minus_one;
	int dim_squared_minus_one;
	float max_elevation_real_y;
	float min_elevation_real_y;
	float elevation_real_diff;

	inline unsigned char get_pixel(int x, int y);

public:
	bool bad() const { return _bad; }
	float lookup(float x, float y);
	HeightMap(const std::string &filename, float _scale, float _top, float _bottom);

};


#endif

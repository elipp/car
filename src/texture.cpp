﻿#include <cstdio>

#include "lodepng.h"
#include "texture.h"
#include "jpeglib.h"

#include "text.h"

static std::string get_file_extension(const std::string &filename) {
	int i = 0;
	size_t fn_len = filename.length();
	while (i < fn_len && filename[i] != '.') {
		++i;
	}
	std::string ext = filename.substr(i+1, filename.length() - (i+1));
	//onScreenLog::print("get_file_extension: \"%s\"\n", ext.c_str());
	return ext;
}

static int loadJPEG(const std::string &filename, unsigned char **out_buffer, unified_header_data *out_header) {

  struct jpeg_decompress_struct cinfo;

  struct jpeg_error_mgr jerr;
  /* More stuff */
  FILE* infile;		/* source file */
  JSAMPROW *buffer;
  int row_stride;		/* physical row width in output buffer */
	int err;
  if ((err = fopen_s(&infile, filename.c_str(), "rb")) != 0) {
    fprintf(stderr, "loadJPEG: fatal: can't open %s\n", filename.c_str());
    return 0;
  }

  /* Step 1: allocate and initialize JPEG decompression object */

  cinfo.err = jpeg_std_error(&jerr);
  /* Establish the setjmp return context for my_error_exit to use. */

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */
  jpeg_stdio_src(&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress(&cinfo);

  /* JSAMPLEs per row in output buffer */
  row_stride = cinfo.output_width * cinfo.output_components;
  unsigned total_bytes = (cinfo.output_width*cinfo.output_components)*(cinfo.output_height*cinfo.output_components);
  *out_buffer = new unsigned char[total_bytes];
  /* Make a one-row-high sample array that will go away when done with image */
  buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  unsigned char* outbuf_iter = *out_buffer;
  while (cinfo.output_scanline < cinfo.output_height) {
    (void) jpeg_read_scanlines(&cinfo, buffer, 1);
    /* Assume put_scanline_someplace wants a pointer and sample count. */
    //put_scanline_someplace(buffer[0], row_stride);
	memcpy(outbuf_iter, buffer[0], row_stride);
	outbuf_iter += row_stride;
  }

  /* Step 7: Finish decompression */
  (void) jpeg_finish_decompress(&cinfo);

  /* Step 8: Release JPEG decompression object */
  
  out_header->width = cinfo.image_width;
  out_header->height = cinfo.image_height;
  out_header->bpp = cinfo.output_components * 8;	// safe assumption
 
  jpeg_destroy_decompress(&cinfo);

  fclose(infile);
  return 1;

}


static int loadPNG(const std::string &filename, unsigned char **out, unified_header_data *out_header) {
	out_header->bpp = 32;	// the below function always decodes to 32-bit RGBA :P
	return (lodepng_decode32_file(out, &out_header->width, &out_header->height, filename.c_str()) == 0); // 0 means no error
}

static int load_pixels(const std::string& filename, unsigned char** pixels, unified_header_data *img_info) {
	unsigned width = 0, height = 0;

	memset(img_info, 0x0, sizeof(*img_info));
	
	std::string ext = get_file_extension(filename);
	if (ext == "jpg" || ext == "jpeg") {
		if (!loadJPEG(filename, pixels, img_info)) {
			fprintf(stderr, "load_pixels: fatal error: loading file %s failed.\n", filename.c_str());
			return 0;
		}
		else {
			return 1;
		}
	
	}
	else if (ext == "png") {
		if(!loadPNG(filename, pixels, img_info)) {
			fprintf(stderr, "load_pixels: fatal error: loading file %s failed.\n", filename.c_str());
			return 0;
		}
		else { 
			return 1;
		}
	} else {
		fprintf(stderr, "load_pixels: fatal error: unsupported image format \"%s\" (only PNG/JPG are supported)\n", ext.c_str());
		return 0;
	}
}


Texture::Texture(const std::string &filename, const GLint filter_param) : name(filename) {

	//haidi haida. oikea kurahaara ja jakorasia.
	
	unified_header_data img_info;

	unsigned char* pixels;
	if (!load_pixels(filename, &pixels, &img_info)) { fprintf(stderr, "Loading texture %s failed.\n", filename.c_str()); _otherbad = true; }

	_badheader = _nosuch = _otherbad = false;


	if (IS_POWER_OF_TWO(img_info.width) == 0 && img_info.width == img_info.height) {
			// image is valid, carry on
			GLint internal_format = GL_RGBA;
			GLint input_pixel_format = img_info.bpp == 32 ? GL_RGBA : GL_RGB;
			glEnable(GL_TEXTURE_2D);
			glGenTextures(1, &textureId);
			glBindTexture( GL_TEXTURE_2D, textureId);
			
			glTexImage2D(GL_TEXTURE_2D, 0, internal_format, img_info.width, img_info.height, 0, input_pixel_format, GL_UNSIGNED_BYTE, (const GLvoid*) &pixels[0]);
			//glTexStorage2D(GL_TEXTURE_2D, 4, internalfmt, width, height); // this is superior to glTexImage2D. only available in GL4 though
			//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid*)&buffer[0]);
			
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_param);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		
		}
	
		else {	// if not power of two
			_otherbad = true;
		}
		
		free(pixels);
}

	/*****
	* REMEMBER: THE OPENGL CONTEXT MUST BE ALREADY CREATED BEFORE ANY OF THE gl<WHATEVER> class take place
	*/

std::vector<Texture> TextureBank::textures;

GLint TextureBank::get_id_by_name(const std::string &name) {
	std::vector<Texture>::const_iterator iter = textures.begin();
	while (iter != textures.end()) {
		const Texture &t = (*iter);
		if (t.getName() == name) {
			return t.getId();
		}
		++iter;
	}
	return -1;
}

GLuint TextureBank::add(const Texture &t) {
	textures.push_back(t);
	return t.getId();
}

bool TextureBank::validate() {
			
			std::vector<Texture>::const_iterator iter;
			bool all_good = true;
			for(iter = textures.begin(); iter != textures.end(); ++iter)
			{	
				const Texture &t = (*iter);
				if (t.bad()) {
					all_good = false;
					onScreenLog::print( "[Textures] invalid textures detected:\n");
					
					if (t.badheader()) {
						onScreenLog::print( "%s: bad file header.\n", t.getName().c_str()); }
					
					else if (t.nosuch())
						onScreenLog::print( "%s: no such file or directory.\n", t.getName().c_str());

					else if (t.otherbad())
						onScreenLog::print( "%s: file either is not square (n-by-n), or not power of two (128x128, 256x256 etc.)\n\n", t.getName().c_str());
					}
					
			}

			return all_good;
}

HeightMap::HeightMap(const std::string &filename, float _scale) : scale(_scale) {
	
	pixels = NULL;
	_bad = false;
	unsigned width = 0, height = 0;
	unified_header_data img_info;

	if (!load_pixels(filename, &this->pixels, &img_info)) {
		fprintf(stderr, "HeightMap: loading file %s pheyled.\n", filename.c_str()); 
		pixels = NULL;
		_bad = true; 
		return; 
	}

	else if (img_info.bpp != 8) {
		fprintf(stderr, "HeightMap: only 8-bit grayscale (jpg) accepted! :(\n", filename.c_str());
	
		pixels = NULL;
		_bad = true;
		return;
	}

	dim = img_info.width;
	
	dim_per_scale = dim/scale;
	half_scale = scale/2.0;
	dim_minus_one = dim-1;
	dim_squared_minus_one = dim*dim - 1;

}

#define Z_VALUE(x, y) ((float)pixels[min((int)(x), dim_minus_one) +\
									 min((int)(y)*dim, dim_squared_minus_one)])

float HeightMap::lookup(float x, float y) {
	// the actual map is this->scale - by - this->scale wide
	x = min(x, scale - 1);	// limit sampled pos-values to scale - 1
	y = min(y, scale - 1);

	// perform bilinear interpolation on the heightmap ^^

	float x_index; float x_frac;
	x_frac = modf((x + half_scale)*dim_per_scale, &x_index);
	
	float y_index; float y_frac;
	y_frac = modf((y + half_scale)*dim_per_scale, &y_index);

	float z11 = Z_VALUE(x_index, y_index);
	float z21 = Z_VALUE(x_index + 1, y_index);
	float z12 = Z_VALUE(x_index, y_index + 1);
	float z22 = Z_VALUE(x_index + 1, y_index + 1);

	
	float one_minus_x_frac = 1.0 - x_frac;
	float one_minus_y_frac = 1.0 - y_frac;

	__m128 c = _mm_setr_ps(z11, z21, z12, z22);
	__m128 d = _mm_setr_ps(x_frac * y_frac, 
						  one_minus_x_frac*y_frac, 
						  x_frac*one_minus_y_frac, 
						  one_minus_x_frac*one_minus_y_frac);

	float r = MM_DPPS_XYZW(c, d);
	
	//onScreenLog::print("lookup: z11 = %f, z21 = %f, z12 = %f, z22 = %f, interpolated value = %f\n", z11, z21, z12, z22, r);
	//return r;
	return z11;
}





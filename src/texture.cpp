#include <cstdio>

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

static int loadJPEG(const std::string &filename, unsigned char **out_buffer, unified_header_data *out_header, int *total_bytes) {

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
  *total_bytes = (cinfo.output_width*cinfo.output_components)*(cinfo.output_height*cinfo.output_components);
  *out_buffer = new unsigned char[*total_bytes];
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

  
  out_header->width = cinfo.image_width;
  out_header->height = cinfo.image_height;
  out_header->bpp = cinfo.output_components * 8;	// safe assumption
 
  /* Step 8: Release JPEG decompression object */
  jpeg_destroy_decompress(&cinfo);

  fclose(infile);
  return 1;

}


static int loadPNG(const std::string &filename, std::vector<unsigned char> &pixels, unified_header_data *out_header, LodePNGColorType colortype) {
	unsigned e = lodepng::decode(pixels, out_header->width, out_header->height, filename, colortype);
	if (colortype == LCT_RGBA) { out_header->bpp = 32; }
	else if (colortype == LCT_GREY) { out_header->bpp = 8; }
	return (e == 0);
}

static int load_pixels(const std::string& filename, std::vector<unsigned char> &pixels, unified_header_data *img_info, LodePNGColorType colortype = LCT_RGBA) {
	unsigned width = 0, height = 0;

	memset(img_info, 0x0, sizeof(*img_info));
	
	
	std::string ext = get_file_extension(filename);
	if (ext == "jpg" || ext == "jpeg") {
		unsigned char* pixel_buf;
		int total_bytes;
		if (!loadJPEG(filename, &pixel_buf, img_info, &total_bytes)) {
			fprintf(stderr, "load_pixels: fatal error: loading file %s failed.\n", filename.c_str());
			return 0;
		}
		else {
			pixels.assign(pixel_buf, pixel_buf + total_bytes);
			delete [] pixel_buf;
			return 1;
		}
	
	}
	else if (ext == "png") {
		if(!loadPNG(filename, pixels, img_info, colortype)) {
			fprintf(stderr, "load_pixels: fatal error: loading file %s failed.\n", filename.c_str());
			return 0;
		}
		else { 
			return 1;
		}
	} else {
		fprintf(stderr, "load_pixels: fatal error: unsupported image file extension \"%s\" (only .png, .jpg, .jpeg are supported)\n", ext.c_str());
		return 0;
	}
}


Texture::Texture(const std::string &filename, const GLint filter_param) : name(filename) {

	//haidi haida. oikea kurahaara ja jakorasia.
	
	unified_header_data img_info;

	std::vector<unsigned char> pixels;
	if (!load_pixels(filename, pixels, &img_info)) { fprintf(stderr, "Loading texture %s failed.\n", filename.c_str()); _otherbad = true; }

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

HeightMap::HeightMap(const std::string &filename, float _scale, float _top, float _bottom) 
	: scale(_scale), top(_top*_scale), bottom(_bottom*_scale), real_map_dim(16 * _scale) {
	
	// the heightmaps are 16-by-16 meshes, which are scaled by the _scale parm

	_bad = false;
	unsigned width = 0, height = 0;
	unified_header_data img_info;

	if (!load_pixels(filename, this->pixels, &img_info, LCT_GREY)) {
		fprintf(stderr, "HeightMap: loading file %s pheyled.\n", filename.c_str()); 
		_bad = true; 
		return; 
	}

	else if (img_info.bpp != 8) {
		fprintf(stderr, "HeightMap: only 8-bit grayscale accepted! :(\n", filename.c_str());

		_bad = true;
		return;
	}

	img_dim_pixels = img_info.width;

	real_map_dim_minus_one = real_map_dim - 1;
	
	dim_per_scale = img_dim_pixels/real_map_dim;
	half_real_map_dim = real_map_dim/2.0;
	dim_minus_one = img_dim_pixels-1;
	dim_squared_minus_one = img_dim_pixels*img_dim_pixels - 1;

	max_elevation_real_y = _top * _scale;
	min_elevation_real_y = _bottom * _scale;

	elevation_real_diff = max_elevation_real_y - min_elevation_real_y;

	onScreenLog::print("HeightMap (filename: %s):\n", filename.c_str());
	onScreenLog::print("img_dim_pixels = %d, dim_per_scale = %f, max_elevation_real_y = %f,\nmin_elevation_real_y = %f, elevation_real_diff = %f\n",
						img_dim_pixels, dim_per_scale, max_elevation_real_y, min_elevation_real_y, elevation_real_diff);
	onScreenLog::print("pixel buffer size: %u\n", this->pixels.size());

}

inline unsigned char HeightMap::get_pixel(int x, int y) {
		int index_x = dim_per_scale*LIMIT_VALUE_BETWEEN((x+half_real_map_dim), 0, real_map_dim_minus_one);
		int index_y = dim_per_scale*LIMIT_VALUE_BETWEEN((y+half_real_map_dim), 0, real_map_dim_minus_one);	
		return pixels[index_x + (dim_minus_one - index_y)*img_dim_pixels];
}

float HeightMap::lookup(float x, float y) {

	// perform bilinear interpolation on the heightmap ^^

	int xi; 
	float xf, xf_r;
	xi = floor(x);
	xf = x - xi;
	xf_r = 1.0 - xf;
	
	int yi; 
	float yf, yf_r;
	yi = floor(y);
	yf = y - yi;
	yf_r = 1.0 - yf;
	
	// this is a simple scalar implementation
	//float R1 = xf_r * z11 + xf * z21;
	//float R2 = xf_r * z12 + xf * z22;
	//float r = (yf_r * R1 + yf * R2)/255.0;
	
	__declspec(align(16)) float zs[4] = { get_pixel(xi,		yi), 
										  get_pixel(xi+1,	yi), 
										  get_pixel(xi,		yi+1), 
										  get_pixel(xi+1,	yi+1) };

	const __m128 z = _mm_load_ps(zs);

	__declspec(align(16)) float weights[4] = { xf_r * yf_r, 
											   xf * yf_r, 
											   xf_r * yf, 
											   xf * yf };
	const __m128 w = _mm_load_ps(weights);

	float r = dot4(z, w)/255.0;
	return min_elevation_real_y + r * elevation_real_diff;
}





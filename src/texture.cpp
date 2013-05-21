#include <cstdio>

#include "lodepng.h"
#include "texture.h"
#include "jpeglib.h"

#include "text.h"


static int loadJPEG(const std::string &filename, unsigned char **out_buffer, unsigned *width, unsigned *height) {

  struct jpeg_decompress_struct cinfo;

  struct jpeg_error_mgr jerr;
  /* More stuff */
  FILE* infile;		/* source file */
  JSAMPROW *buffer;
  int row_stride;		/* physical row width in output buffer */
	
  if ((infile = fopen(filename.c_str(), "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename.c_str());
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
  jpeg_destroy_decompress(&cinfo);

  fclose(infile);
  *width = cinfo.image_width;
  *height = cinfo.image_height;
  return 1;

}


static int loadPNG(const std::string &filename, unsigned char **out, unsigned *width, unsigned *height) {
	return (lodepng_decode32_file(out, width, height, filename.c_str()) == 0); // 0 means no error
	// TODO need to free the buffers!!
}


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
Texture::Texture(const std::string &filename, const GLint filter_param) : name(filename)
{

	//haidi haida. oikea kurahaara ja jakorasia.

	unsigned char *buffer = NULL;
	unsigned width = 0, height = 0;
	
	std::string ext = get_file_extension(filename);
	if (ext == "jpg" || ext == "jpeg") {
		hasAlpha = false;
		if (!loadJPEG(filename, &buffer, &width, &height)) {
			onScreenLog::print("Texture: fatal error: loading file %s failed.\n", filename.c_str());
			_otherbad = true;
		}
	
	}
	else if (ext == "png") {
		hasAlpha = true;
		if(!loadPNG(filename, &buffer, &width, &height)) {
			onScreenLog::print("Texture: fatal error: loading file %s failed.\n", filename.c_str());
			_otherbad=true;
		}
		_otherbad=true;
	} else {
		onScreenLog::print("Texture: fatal error: unsupported image format \"%s\" (only PNG/JPG are supported)\n", ext.c_str());
		_otherbad = true; 
		return; 
	}
	
	_badheader = _nosuch = _otherbad = false;

	if ((width & (width - 1)) == 0 && width == height) {
			// image is valid, carry on
			GLint internalfmt = hasAlpha ? GL_RGBA8 : GL_RGB8;
			GLint texSubfmt = hasAlpha ? GL_RGBA : GL_RGB;
			glEnable(GL_TEXTURE_2D);
			glGenTextures(1, &textureId);
			glBindTexture( GL_TEXTURE_2D, textureId);
			glTexStorage2D(GL_TEXTURE_2D, 4, internalfmt, width, height);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, texSubfmt, GL_UNSIGNED_BYTE, (const GLvoid*)&buffer[0]);
			glGenerateMipmap(GL_TEXTURE_2D);	// requires glew
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_param);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
	
		else {	// if not power of two
			_otherbad = true;
		}

		//free(buffer);
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

void TextureBank::add(const Texture &t) {
	textures.push_back(t);
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





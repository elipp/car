#include "common.h"


/**
 * NOTE: THESE TWO FUNCTIONS ALSO RESET THE INTERNAL POINTERS TO THE 0-offset position.
 */

size_t getfilesize(FILE *file)
{
	fseek(file, 0, SEEK_END);
	size_t filesize = ftell(file);
	rewind(file);

	return filesize;
}


size_t cpp_getfilesize(std::ifstream& in)
{
	in.seekg (0, std::ios::end);
	long length = in.tellg();
	in.seekg(0, std::ios::beg);

	return length;
}

/*
char* decompress_qlz(std::ifstream &file, char** buffer)
{

	
	qlz_state_decompress *state = new qlz_state_decompress;
	long infile_size = cpp_getfilesize(file);
	char* compressed_buf = new char[infile_size];
	file.read(compressed_buf, infile_size);
	
	std::size_t uncompressed_size = qlz_size_decompressed(compressed_buf);

	*buffer = new char[uncompressed_size];

	std::size_t filesize = qlz_decompress(compressed_buf, *buffer, state);

	delete [] compressed_buf;
	delete state;

	return *buffer;

}*/


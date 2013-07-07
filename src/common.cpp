#include "common.h"

const float PI_PER_TWO = M_PI/2;

const float PI_PER_180 = (M_PI/180.0);
const float _180_PER_PI = (180.0/M_PI);

static std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}



std::string get_timestamp() {
	char buffer[128];
	SYSTEMTIME st;
    GetLocalTime(&st);
	sprintf_s(buffer, "%02d:%02d", st.wHour, st.wMinute);
	return std::string(buffer);
}

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


#include "common.h"

bool keys[256] = { false };

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

static const float FW_ANGLE_LIMIT = M_PI/6; // rad
static const float SQRT_FW_ANGLE_LIMIT_RECIP = 1.0/sqrt(FW_ANGLE_LIMIT);
// f(x) = -(1/(x+1/sqrt(30))^2) + 30
// f(x) = 1/(-x + 1/sqrt(30))^2 - 30

float f_wheel_angle(float x) {
	float t;
	if (x >= 0) {
		t = x+SQRT_FW_ANGLE_LIMIT_RECIP;
		t *= t;
		return -(1/t) + FW_ANGLE_LIMIT;
	}
	else {
		t = -x+SQRT_FW_ANGLE_LIMIT_RECIP;
		t *= t;
		return (1/t) - FW_ANGLE_LIMIT;
	}
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


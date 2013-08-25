#ifndef SHADER_H
#define SHADER_H

#include "glwindow_win32.h"

#include <fstream>
#include <cstring> // for memset and stuff
#include <string>
#include <iostream>
#include <algorithm>
#include <unordered_map>

#define SHADER_NONE (GLuint)-1
#define SHADER_SUCCESS GL_TRUE

enum {
	ATTRIB_POSITION = 0,
	ATTRIB_NORMAL = 1,
	ATTRIB_TEXCOORD = 2
};

#ifndef GL_TESS_EVALUATION_SHADER
#define GL_TESS_EVALUATION_SHADER 0x0
#endif

#ifndef GL_TESS_CONTROL_SHADER
#define GL_TESS_CONTROL_SHADER 0x0
#endif



typedef enum { 
		VertexShader = 0, 
		TessellationControlShader = 1, 
		TessellationEvaluationShader = 2, 
		GeometryShader = 3, 
		FragmentShader = 4
};

#define set_bad() do {\
	bad = true;\
	PRINT("Program %s: bad flag set @ %s:%d\n", id_string.c_str(), __FILE__, __LINE__);\
} while(0)\
	
class ShaderProgram {
	std::unordered_map<std::string, GLuint> uniforms;	// uniform name -> uniform location
	std::string id_string;
	std::string shader_filenames[5];
	GLuint programHandle;
	GLuint shaderObjIDs[5]; 	// [0] => VS_id, [1] => TCS_id, [2] => TES_id, [3] => GS_id, [4] => FS_id
	bool bad;
public:
	GLuint getProgramHandle() const { return programHandle; }
	
	ShaderProgram(const std::string &name_base, const std::unordered_map<GLuint, std::string> &bindattrib_loc_names_map); // extensions are appended to the name base, see shader.cpp

	void printStatus() const;
	GLint checkShaderCompileStatus_all();
	GLint checkProgramLinkStatus();
	bool is_bad() const { return bad; }

	void construct_uniform_map();
	void update_uniform_mat4(const std::string &uniform_name, const mat4 &m);
	void update_uniform_vec4(const std::string &uniform_name, const vec4 &v);
	void update_uniform_1f(const std::string &uniform_name, GLfloat value);
	void update_uniform_1i(const std::string &uniform_name, GLint value);	// just wrappers around the glapi calls

	static char* readShaderFromFile(const std::string &filename, GLsizei *filesize);
	std::string get_id_string() const { return id_string; }
	std::string get_vs_filename() const { return shader_filenames[VertexShader]; }
	std::string get_tcs_filename() const { return shader_filenames[TessellationControlShader]; }
	std::string get_tes_filename() const { return shader_filenames[TessellationEvaluationShader]; }
	std::string get_gs_filename() const { return shader_filenames[GeometryShader]; }
	std::string get_fs_filename() const { return shader_filenames[FragmentShader]; }
};


#endif

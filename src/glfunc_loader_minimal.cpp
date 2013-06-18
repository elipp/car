#include "glfunc_loader_minimal.h"

#include <cstdio>

int		(*wglSwapIntervalEXT)(int);

void	(*glGenBuffers)(GLsizei, GLuint*);	 // (n, buffers)
void	(*glBindBuffer)(GLenum, GLuint);	// (target, buffer)
void	(*glBufferData)(GLenum, GLsizeiptr, const GLvoid*, GLenum); // (target, size, data, usage)
void	(*glBufferSubData)(GLenum, GLintptr, GLsizeiptr, const GLvoid*); // (target, offset, size, data)
GLint	(*glGetUniformLocation)(GLuint, const GLchar*); // (program, name)

void	(*glGetActiveUniform) (GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*); // (program, index, bufSize, length, size, type, name)
void	(*glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*); // (program, maxLength, length, infoLog)
GLuint	(*glCreateShader)(GLenum); // (shaderType)
void	(*glAttachShader)(GLuint, GLuint); // (program, shader)
void	(*glShaderSource)(GLuint, GLsizei, const GLchar **, const GLint*); // (shader, count, string, length)
void	(*glCompileShader)(GLuint); // (shader)
void	(*glLinkProgram)(GLuint); // program
GLuint	(*glCreateProgram)(void); // ()
void	(*glValidateProgram)(GLuint); // (program)
void	(*glUseProgram)(GLuint); // (program)

void	(*glGetProgramiv)(GLuint, GLenum, GLint*); // (program, pname, params)
void	(*glGetShaderInfoLog)(GLuint, GLsizei, GLsizei, GLchar*); // (shader, maxLength, length, infoLog)
void	(*glGetShaderiv)(GLuint, GLenum, GLint*); // (shader, pname, params)

void	(*glUniform1f)(GLint, GLfloat); // (location, val)
void	(*glUniform1i)(GLint, GLint); // (location, val)
void	(*glUniform4fv)(GLint, GLsizei, const GLfloat*); // (location, count, value)
void	(*glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*); // (location, count, transpose, val_arr)

void	(*glEnableVertexAttribArray)(GLuint); // (index)
void	(*glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*); // (index, size, type, normalized, stride, ptr)
void	(*glBindAttribLocation)(GLuint, GLuint, const GLchar*); // (program, index, name)

void	(*glGenerateMipmap)(GLenum); // (target)
void	(*glActiveTexture)(GLenum) ; // (texture)

// oh man, decltype <3
#define _LOADFUNC(FUNCNAME) do {\
	FUNCNAME = (decltype(FUNCNAME))wglGetProcAddress(#FUNCNAME);\
	if (FUNCNAME == NULL) {\
		int e = GetLastError();\
		fprintf(stderr, "GLfunc_loader_minimal: failure loading extfunc %s!\n", #FUNCNAME);\
		++num_failed;\
		}\
	} while(0)

int load_functions() {

	int num_failed = 0;

	_LOADFUNC(wglSwapIntervalEXT);

	_LOADFUNC(glGenBuffers);
	_LOADFUNC(glBindBuffer);
	_LOADFUNC(glBufferData);
	_LOADFUNC(glBufferSubData);

	_LOADFUNC(glCreateShader);
	_LOADFUNC(glAttachShader);
	_LOADFUNC(glShaderSource);
	_LOADFUNC(glCompileShader);
	_LOADFUNC(glCreateProgram);
	_LOADFUNC(glValidateProgram);
	_LOADFUNC(glUseProgram);
	_LOADFUNC(glLinkProgram);

	_LOADFUNC(glGetProgramInfoLog);
	_LOADFUNC(glGetActiveUniform);
	_LOADFUNC(glGetUniformLocation);
	_LOADFUNC(glGetProgramiv);
	_LOADFUNC(glGetShaderiv);

	_LOADFUNC(glUniform1f);
	_LOADFUNC(glUniform1i);
	_LOADFUNC(glUniform4fv);
	_LOADFUNC(glUniformMatrix4fv);

	_LOADFUNC(glEnableVertexAttribArray);
	_LOADFUNC(glVertexAttribPointer);
	_LOADFUNC(glBindAttribLocation);

	_LOADFUNC(glGenerateMipmap);
	_LOADFUNC(glActiveTexture);

	return num_failed;

}
#include "glfunc_loader_minimal.h"

#include <cstdio>

int		(*wglSwapIntervalEXT)(int);

void    (APIENTRY *glGenBuffers)(GLsizei, GLuint*);	 // (n, buffers)
void    (APIENTRY *glBindBuffer) (GLenum target, GLuint buffer);
void	(APIENTRY *glBufferData)(GLenum, GLsizeiptr, const GLvoid*, GLenum); // (target, size, data, usage)
void	(APIENTRY *glBufferSubData)(GLenum, GLintptr, GLsizeiptr, const GLvoid*); // (target, offset, size, data)
GLint	(APIENTRY *glGetUniformLocation)(GLuint, const GLchar*); // (program, name)

void	(APIENTRY *glGetActiveUniform) (GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*); // (program, index, bufSize, length, size, type, name)
void	(APIENTRY *glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*); // (program, maxLength, length, infoLog)
GLuint	(APIENTRY *glCreateShader)(GLenum); // (shaderType)
void	(APIENTRY *glAttachShader)(GLuint, GLuint); // (program, shader)
void    (APIENTRY *glShaderSource) (GLuint shader, GLsizei count, const GLchar* const *string, const GLint *length);
void	(APIENTRY *glCompileShader)(GLuint); // (shader)
void	(APIENTRY *glLinkProgram)(GLuint); // program
GLuint	(APIENTRY *glCreateProgram)(void); // ()
void	(APIENTRY *glValidateProgram)(GLuint); // (program)
void	(APIENTRY *glUseProgram)(GLuint); // (program)

void	(APIENTRY *glGetProgramiv)(GLuint, GLenum, GLint*); // (program, pname, params)
void	(APIENTRY *glGetShaderInfoLog) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
void	(APIENTRY*glGetShaderiv)(GLuint, GLenum, GLint*); // (shader, pname, params)

void	(APIENTRY *glUniform1f)(GLint, GLfloat); // (location, val)
void	(APIENTRY *glUniform1i)(GLint, GLint); // (location, val)
void	(APIENTRY *glUniform4fv)(GLint, GLsizei, const GLfloat*); // (location, count, value)
void	(APIENTRY *glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*); // (location, count, transpose, val_arr)

void	(APIENTRY *glEnableVertexAttribArray)(GLuint); // (index)
void	(APIENTRY *glDisableVertexAttribArray) (GLuint); // (index)
void	(APIENTRY *glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*); // (index, size, type, normalized, stride, ptr)
void	(APIENTRY *glBindAttribLocation)(GLuint, GLuint, const GLchar*); // (program, index, name)

void	(APIENTRY *glGenerateMipmap)(GLenum); // (target)
void	(APIENTRY *glActiveTexture)(GLenum) ; // (texture)
void 	(APIENTRY *glGenVertexArrays) (GLsizei n, GLuint *arrays);
void	(APIENTRY *glBindVertexArray)(GLuint); // (array)

// oh man, decltype <3
#define _LOADFUNC(FUNCNAME) do {\
	FUNCNAME = (decltype(FUNCNAME))wglGetProcAddress(#FUNCNAME);\
	if (FUNCNAME == NULL) {\
		int e = GetLastError();\
		fprintf(stderr, "GLfunc_loader_minimal: failure loading extfunc %s!\n", #FUNCNAME);\
		++num_failed;\
		}\
	else { fprintf(stderr, "Successfully loaded function %s (addr = %p).\n", #FUNCNAME, FUNCNAME); }\
	} while(0)

int load_GL_functions() {

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
	_LOADFUNC(glDisableVertexAttribArray);
	_LOADFUNC(glVertexAttribPointer);
	_LOADFUNC(glBindAttribLocation);

	_LOADFUNC(glGenerateMipmap);
	_LOADFUNC(glActiveTexture);

	_LOADFUNC(glGenVertexArrays);
	_LOADFUNC(glBindVertexArray);

	return num_failed;

}
#ifndef GLFUNC_LOADER_MINIMAL_H
#define GLFUNC_LOADER_MINIMAL_H

//http://www.opengl.org/registry/api/GL/glext.h

#include "glwindow_win32.h"

typedef char GLchar;
typedef int GLsizei;

typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;

#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_ACTIVE_UNIFORMS                0x8B86
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_COMPILE_STATUS                 0x8B81
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_GEOMETRY_SHADER                0x8DD9
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_LINK_STATUS                    0x8B82
#define GL_SHADING_LANGUAGE_VERSION       0x8B8C
#define GL_STATIC_DRAW                    0x88E4
#define GL_TEXTURE0                       0x84C0
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_GEOMETRY_SHADER                0x8DD9
#define GL_TESS_CONTROL_SHADER            0x8E88
#define GL_TESS_EVALUATION_SHADER         0x8E87
#define GL_ACTIVE_UNIFORMS                0x8B86
#define GL_COMPILE_STATUS                 0x8B81
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_PROGRAM_POINT_SIZE             0x8642

extern int load_functions();

extern int (*wglSwapIntervalEXT)(int);

extern void (*glGenBuffers)(GLsizei, GLuint*);	 // (n, buffers)
extern void (*glBindBuffer)(GLenum, GLuint);	// (target, buffer)
extern void (*glBufferData)(GLenum, GLsizeiptr, const GLvoid*, GLenum); // (target, size, data, usage)
extern void (*glBufferSubData)(GLenum, GLintptr, GLsizeiptr, const GLvoid*); // (target, offset, size, data)
extern GLint (*glGetUniformLocation)(GLuint, const GLchar*); // (program, name)

extern void (*glGetActiveUniform) (GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*); // (program, index, bufSize, length, size, type, name)
extern void (*glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*); // (program, maxLength, length, infoLog)
extern GLuint (*glCreateShader)(GLenum); // (shaderType)
extern void (*glAttachShader)(GLuint, GLuint); // (program, shader)
extern void (*glShaderSource)(GLuint, GLsizei, const GLchar **, const GLint*); // (shader, count, string, length)
extern void (*glCompileShader)(GLuint); // (shader)
extern void (*glLinkProgram)(GLuint); // program
extern GLuint (*glCreateProgram)(void); // ()
extern void (*glValidateProgram)(GLuint); // (program)
extern void (*glUseProgram)(GLuint); // (program)

extern void (*glGetProgramiv)(GLuint, GLenum, GLint*); // (program, pname, params)
extern void (*glGetShaderInfoLog)(GLuint, GLsizei, GLsizei, GLchar*); // (shader, maxLength, length, infoLog)
extern void (*glGetShaderiv)(GLuint, GLenum, GLint*); // (shader, pname, params)

extern void (*glUniform1f)(GLint, GLfloat); // (location, val)
extern void (*glUniform1i)(GLint, GLint); // (location, val)
extern void (*glUniform4fv)(GLint, GLsizei, const GLfloat*); // (location, count, value)
extern void (*glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*); // (location, count, transpose, val_arr)

extern void (*glEnableVertexAttribArray)(GLuint); // (index)
extern void (*glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*); // (index, size, type, normalized, stride, ptr)
extern void (*glBindAttribLocation)(GLuint, GLuint, const GLchar*); // (program, index, name)

extern void (*glGenerateMipmap)(GLenum); // (target)
extern void (*glActiveTexture)(GLenum) ; // (texture)

#endif
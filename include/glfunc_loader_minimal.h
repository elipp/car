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

extern int load_GL_functions();

extern int (*wglSwapIntervalEXT)(int);
#define GLAPI extern

GLAPI void (APIENTRY *glGenBuffers)(GLsizei n, GLuint* buffers);	 // (n, buffers)
GLAPI void (APIENTRY *glBindBuffer) (GLenum target, GLuint buffer);
GLAPI void (APIENTRY *glBufferData)(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage); // (target, size, data, usage)
GLAPI void (APIENTRY *glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data); // (target, offset, size, data)

GLAPI void (APIENTRY *glGetProgramInfoLog) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
GLAPI GLuint (APIENTRY *glCreateProgram)(void); // ()
GLAPI GLuint (APIENTRY *glCreateShader) (GLenum type);
GLAPI void (APIENTRY *glAttachShader)(GLuint program, GLuint shader); // (program, shader)
GLAPI void (APIENTRY *glShaderSource) (GLuint shader, GLsizei count, const GLchar* const *string, const GLint *length);
GLAPI void (APIENTRY *glCompileShader)(GLuint shader); // (shader)
GLAPI void (APIENTRY *glLinkProgram)(GLuint program); // program
GLAPI void (APIENTRY *glValidateProgram)(GLuint program); // (program)
GLAPI void (APIENTRY *glUseProgram)(GLuint program); // (program)

GLAPI void (APIENTRY *glGetProgramiv) (GLuint program, GLenum pname, GLint *params);
GLAPI void	(APIENTRY *glGetShaderInfoLog) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
GLAPI void (APIENTRY *glGetShaderiv) (GLuint shader, GLenum pname, GLint *params);

GLAPI void (APIENTRY *glGetActiveUniform)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
GLAPI GLint (APIENTRY *glGetUniformLocation)(GLuint program, const GLchar* name); // (program, name)
GLAPI void (APIENTRY *glUniform1f)(GLint location, GLfloat val); // (location, val)
GLAPI void (APIENTRY *glUniform1i)(GLint location, GLint val); // (location, val)
GLAPI void (APIENTRY *glUniform4fv)(GLint location, GLsizei count, const GLfloat* value); // (location, count, value)
GLAPI void (APIENTRY *glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* val_arr); // (location, count, transpose, val_arr)

GLAPI void (APIENTRY *glEnableVertexAttribArray)(GLuint index); // (index)
GLAPI void (APIENTRY *glDisableVertexAttribArray) (GLuint index); // (index)
GLAPI void (APIENTRY *glVertexAttribPointer) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
GLAPI void (APIENTRY *glBindAttribLocation) (GLuint program, GLuint index, const GLchar *name);

GLAPI void (APIENTRY *glGenerateMipmap)(GLenum target); // (target)
GLAPI void (APIENTRY *glActiveTexture)(GLenum texture) ; // (texture)

GLAPI void (APIENTRY *glGenVertexArrays) (GLsizei n, GLuint *arrays);
GLAPI void (APIENTRY *glBindVertexArray)(GLuint array); // (array)

#endif
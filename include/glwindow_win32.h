#ifndef GLWINDOW_WIN32_H
#define GLWINDOW_WIN32_H

//#pragma warning(disable: 4267)
#pragma warning(disable: 4244) // double -> const float conversion warning
#pragma warning(disable: 4305) // double -> const float trunc warning

#define NOMINMAX	// this prevents minwindef.h from defining those name-clashing min/max macros (std::numeric_limits<>::max)

#define MINIMUM(a, b) (((a) < (b)) ? (a) : (b))
#define MAXIMUM(a, b) (((a) > (b)) ? (a) : (b))


#include <Winsock2.h>
#include <Windows.h>

#define GLEW_STATIC
#include <GL\glew.h>
#include <GL\wglew.h>

#include <wingdi.h>
#include <mutex>

#include <lmcons.h>

extern bool WM_KEYDOWN_KEYS[];
extern bool WM_CHAR_KEYS[];

extern void stop_main_loop();
extern void handle_WM_CHAR(WPARAM wParam);
extern void handle_WM_KEYDOWN(WPARAM wParam);
extern void resize_GL_scene(GLsizei width, GLsizei height);
extern bool main_loop_running();
extern void messagebox_error(const std::string &msg);

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

//typedef bool (APIENTRY *PFNWGLSWAPINTERVALEXTPROC) (int interval);
//extern PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

extern unsigned WINDOW_WIDTH;
extern unsigned WINDOW_HEIGHT;
extern float PROJ_FOV_RADIANS;
extern float PROJ_Z_FAR;

#define HALF_WINDOW_WIDTH (WINDOW_WIDTH/2)
#define HALF_WINDOW_HEIGHT (WINDOW_HEIGHT/2)

extern void set_cursor_relative_pos(int x, int y);

extern void window_swapbuffers();
extern bool mouse_locked;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag, HINSTANCE hInstance, int nCmdShow);
void KillGLWindow(void);

#endif
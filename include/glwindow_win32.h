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

#define KEY_Q 'Q'
#define KEY_W 'W'
#define KEY_E 'E'
#define KEY_R 'R'
#define KEY_T 'T'
#define KEY_Y 'Y'
#define KEY_U 'U'
#define KEY_I 'I'
#define KEY_O 'O'
#define KEY_P 'P'
#define KEY_A 'A'
#define KEY_S 'S'
#define KEY_D 'D'
#define KEY_F 'F'
#define KEY_G 'G'
#define KEY_H 'H'
#define KEY_J 'J'
#define KEY_K 'K'
#define KEY_L 'L'
#define KEY_M 'M'
#define KEY_N 'N'
#define KEY_Z 'Z'
#define KEY_X 'X'
#define KEY_C 'C'
#define KEY_V 'V'
#define KEY_B 'B'
#define KEY_N 'N'
#define KEY_M 'M'

extern struct key_mgr keys;

typedef struct {
	unsigned x, unsigned y;
} mouse_pos_t;

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
extern mouse_pos_t get_cursor_pos();

extern void window_swapbuffers();
extern bool mouse_locked;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int create_GL_window(const char* title, int width, int height);
void kill_GL_window(void);

#endif

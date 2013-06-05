#ifndef GLWINDOW_WIN32_H
#define GLWINDOW_WIN32_H

#include <Winsock2.h>
#include <Windows.h>
#include <wingdi.h>
#include <mutex>

#include <gl_core_3_2.h>

extern bool WM_KEYDOWN_KEYS[];
extern bool WM_CHAR_KEYS[];

extern void stop_main_loop();
extern bool main_loop_running();
extern void messagebox_error(const std::string &msg);

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

typedef bool (APIENTRY *PFNWGLSWAPINTERVALEXTPROC) (int interval);
extern PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

extern const float WINDOW_WIDTH;
extern const float WINDOW_HEIGHT;
extern const float HALF_WINDOW_WIDTH;
extern const float HALF_WINDOW_HEIGHT;

extern void set_cursor_relative_pos(int x, int y);

extern void window_swapbuffers();
extern bool mouse_locked;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc_child(HWND, UINT, WPARAM, LPARAM);

BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag);
void KillGLWindow(void);
GLvoid ResizeGLScene(GLsizei width, GLsizei height);

#endif
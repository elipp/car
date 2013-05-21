#ifndef GLWINDOW_WIN32_H
#define GLWINDOW_WIN32_H

#include <Winsock2.h>
#include <Windows.h>
#include <wingdi.h>

#include <gl_core_4_2.h>


typedef bool (APIENTRY *PFNWGLSWAPINTERVALEXTPROC) (int interval);
extern PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

static const float WINDOW_WIDTH = 1440.0;
static const float WINDOW_HEIGHT = 960.0;
static const float HALF_WINDOW_WIDTH = WINDOW_WIDTH/2.0;
static const float HALF_WINDOW_HEIGHT = WINDOW_HEIGHT/2.0;

void window_swapbuffers();

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc_child(HWND, UINT, WPARAM, LPARAM);

BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag);
void KillGLWindow(void);
GLvoid ResizeGLScene(GLsizei width, GLsizei height);

static void clearLogWindow();

#endif
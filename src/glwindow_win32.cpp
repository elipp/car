#include "glwindow_win32.h"

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <stdarg.h>

#include <cassert>
#include <signal.h>
#include <mutex>

#include "common.h"

bool WM_KEYDOWN_KEYS[256] = { false };
bool WM_CHAR_KEYS[256] = { false };

bool mouse_locked = false;

static HGLRC hRC = NULL;
static HDC hDC	  = NULL;
static HWND hWnd = NULL;
static HINSTANCE hInstance;

static HWND hWnd_child = NULL;

float WINDOW_WIDTH = 1280;
float WINDOW_HEIGHT = 960;

bool fullscreen = false;
bool active = TRUE;

extern int initGL();
extern mat4 projection;


static bool _main_loop_running=true;
bool main_loop_running() { return _main_loop_running; }
void stop_main_loop() { _main_loop_running = false; }

void set_cursor_relative_pos(int x, int y) {
    POINT pt;
    pt.x = x;
    pt.y = y;
    ClientToScreen(hWnd, &pt);
    SetCursorPos(pt.x, pt.y);
}


static std::string *convertLF_to_CRLF(const char *buf);

void window_swapbuffers() {
	SwapBuffers(hDC);
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	#define BIT_SET(var,pos) ((var) & (1<<(pos)))
	switch(uMsg)
	{
	case WM_ACTIVATE:
		if(!HIWORD(wParam)) { active=TRUE; }
		else { 
			active = FALSE; 
			mouse_locked = false;
		}
		break;

	case WM_SYSCOMMAND:
		switch(wParam)
		{
			case SC_SCREENSAVE:
			case SC_KEYMENU:
				if ((lParam>>16)<=0) return 0;
				else return DefWindowProc(hWnd, uMsg, wParam, lParam);
			case SC_MONITORPOWER:
				return 0;
		}
		break;

	case WM_CLOSE:
		KillGLWindow();
		PostQuitMessage(0);
		break;

	case WM_CHAR:
		if (onScreenLog::input_field.enabled()) {
			if (wParam != VK_RETURN) { // is handled in the WM_KEYDOWN -caseblock
				onScreenLog::input_field.insert_char_to_cursor_pos(wParam);
			}
		}
		break;
		
	case WM_KEYDOWN:
		if (onScreenLog::input_field.enabled()) {
			if (wParam == VK_RETURN) {
				
				onScreenLog::input_field.submit_and_parse();
				onScreenLog::input_field.disable();
			}
			else if (wParam == VK_ESCAPE) {
				onScreenLog::input_field.disable();
			}
			else if (wParam == VK_LEFT) {
				onScreenLog::input_field.move_cursor(-1);
			}
			else if (wParam == VK_RIGHT) {
				onScreenLog::input_field.move_cursor(1);
			}
			
		}
		else if (wParam == VK_RETURN) {
			onScreenLog::input_field.enable();
		} else {
			WM_KEYDOWN_KEYS[wParam]=TRUE;
		}
		break;

	case WM_KEYUP:
		WM_KEYDOWN_KEYS[wParam]=FALSE;
		break;

	case WM_SIZE:
		ResizeGLScene(LOWORD(lParam), HIWORD(lParam));
		break;
	
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


void messagebox_error(const std::string &msg) {
	MessageBox(NULL, msg.c_str(), "Error (fatal)", MB_OK | MB_ICONEXCLAMATION);
}
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag, HINSTANCE _hInstance, int nCmdShow)
{
	hInstance = _hInstance;

	GLuint PixelFormat;
	WNDCLASS wc;
	DWORD dwExStyle;
	DWORD dwStyle;

	RECT WindowRect;
	WindowRect.left=(long)0;
	WindowRect.right=(long)width;
	WindowRect.top=(long)0;
	WindowRect.bottom=(long)height;


	fullscreen = fullscreenflag;

	//hInstance = GetModuleHandle(NULL);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC) WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "OpenGL";

	if (!RegisterClass(&wc))
	{
		MessageBox(NULL, "FAILED TO REGISTER THE WINDOW CLASS.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	DEVMODE dmScreenSettings;
	memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
	dmScreenSettings.dmSize = sizeof(dmScreenSettings);
	dmScreenSettings.dmPelsWidth = width;
	dmScreenSettings.dmPelsHeight = height;
	dmScreenSettings.dmBitsPerPel = bits;
	dmScreenSettings.dmFields= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

	/*
	 * no need to test this now that fullscreen is turned off
	 *
	if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
	{
		if (MessageBox(NULL, "The requested fullscreen mode is not supported by\nyour video card. Use Windowed mode instead?", "warn", MB_YESNO | MB_ICONEXCLAMATION)==IDYES)
		{
			fullscreen=FALSE;
		}
		else {

			MessageBox(NULL, "Program willl now close.", "ERROR", MB_OK|MB_ICONSTOP);
			return FALSE;
		}
	}*/

	if (fullscreen)
	{
		dwExStyle=WS_EX_APPWINDOW;
		dwStyle=WS_POPUP;

	}

	else {
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle=WS_OVERLAPPEDWINDOW;
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

	hWnd = CreateWindowEx(dwExStyle, "OpenGL", title, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dwStyle, 0, 0, 
		WindowRect.right-WindowRect.left, WindowRect.bottom-WindowRect.top,
		NULL, NULL, hInstance, NULL);

	if (hWnd == NULL) {
		KillGLWindow();
		MessageBox(NULL, "window creation error.", "ERROR", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}


	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		bits,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	if (!(hDC=GetDC(hWnd)))
	{
		KillGLWindow();
		MessageBox(NULL, "CANT CREATE A GL DEVICE CONTEXT.", "ERROR", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!(PixelFormat = ChoosePixelFormat(hDC, &pfd)))
	{
		KillGLWindow();
		MessageBox(NULL, "cant find a suitable pixelformat.", "ERROUE", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}


	if(!SetPixelFormat(hDC, PixelFormat, &pfd))
	{
		KillGLWindow();
		MessageBox(NULL, "Can't SET ZE PIXEL FORMAT.", "ERROU", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if(!(hRC=wglCreateContext(hDC)))
	{
		KillGLWindow();
		MessageBox(NULL, "WGLCREATECONTEXT FAILED.", "ERREUHX", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if(!wglMakeCurrent(hDC, hRC))
	{
		KillGLWindow();
		MessageBox(NULL, "Can't activate the gl rendering context.", "ERAIX", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	// apparently, the WINAPI ShowWindow calls some opengl functions and causes a crash if the funcptrs aren't loaded
	if (load_functions() > 0) {
		messagebox_error("error: load_functions() pheyled!");
		return 0;
	}

//	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress ("wglSwapIntervalEXT");  

	ShowWindow(hWnd, nCmdShow);
	//SetForegroundWindow(hWnd);
	//SetFocus(hWnd);

	return TRUE;
}

void KillGLWindow(void)
{
	if(hRC)
	{
		if(!wglMakeCurrent(NULL,NULL))
		{
			MessageBox(NULL, "wglMakeCurrent(NULL,NULL) failed", "erreur", MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))
		{
			MessageBox(NULL, "RELEASE of rendering context failed.", "error", MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;

		if(hDC && !ReleaseDC(hWnd, hDC))
		{
			MessageBox(NULL, "Release DC failed.", "ERREUX", MB_OK | MB_ICONINFORMATION);
			hDC=NULL;
		}

		if(hWnd && !DestroyWindow(hWnd))
		{
			MessageBox(NULL, "couldn't release hWnd.", "erruexz", MB_OK|MB_ICONINFORMATION);
			hWnd=NULL;
		}

		if (!UnregisterClass("OpenGL", hInstance))
		{
			MessageBox(NULL, "couldn't unregister class.", "err", MB_OK | MB_ICONINFORMATION);
			hInstance=NULL;
		}

	}

}

static std::string *convertLF_to_CRLF(const char *buf) {
	std::string *buffer = new std::string(buf);
	size_t start_pos = 0;
	static const std::string LF = "\n";
	static const std::string CRLF = "\r\n";
    while((start_pos = buffer->find(LF, start_pos)) != std::string::npos) {
        buffer->replace(start_pos, LF.length(), CRLF);
        start_pos += LF.length()+1; // +1 to avoid the new \n we just created :P
    }
	return buffer;
}

GLvoid ResizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	height = max(height, 1);

	WINDOW_WIDTH = width;
	WINDOW_HEIGHT = height;

	glViewport(0,0, WINDOW_WIDTH, WINDOW_HEIGHT);						// Reset The Current Viewport

	text_set_Projection(mat4::proj_ortho(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0, -1.0, 1.0));
	projection = mat4::proj_persp(PROJ_FOV_RADIANS, (WINDOW_WIDTH/WINDOW_HEIGHT), 4.0, PROJ_Z_FAR);
	onScreenLog::input_field.update_position();
	onScreenLog::update_overlay_pos();
	VarTracker::update_position();

}
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <stdarg.h>

#include <cassert>
#include <signal.h>

#include "glwindow_win32.h"

#include "net/server.h"
#include "net/client.h"

#include "lin_alg.h"
#include "common.h"
#include "objloader.h"
#include "shader.h"
#include "texture.h"
#include "text.h"
#include "vertex.h"

static GLint PMODE = GL_FILL;	// polygon mode toggle
static LPPOINT cursorPos = new POINT;	/* holds cursor position */

extern bool active;

float c_vel_fwd = 0, c_vel_side = 0;

struct meshinfo {
	GLuint VBOid;
	GLuint facecount;
};

struct meshinfo chassis, wheel, plane;

GLuint IBOid, FBOid, FBO_textureId;

bool mouseLocked = false;

GLfloat running = 0.0;

static ShaderProgram *regular_shader = NULL;
static ShaderProgram *normal_plot_shader = NULL;

mat4 view;
Quaternion viewq;
static float qx = 0, qy = 0;
mat4 projection;
vec4 view_position;
vec4 cameraVel;

static Car local_car;

GLushort * indices; 

#ifndef M_PI
#define M_PI 3.1415926535
#endif


void rotatex(float mod) {
	qy += mod;
	Quaternion xq = Quaternion::fromAxisAngle(1.0, 0.0, 0.0, qx);
	Quaternion yq = Quaternion::fromAxisAngle(0.0, 1.0, 0.0, qy);
	viewq = yq * xq;
}

void rotatey(float mod) {
	qx -= mod;
	Quaternion xq = Quaternion::fromAxisAngle(1.0, 0.0, 0.0, qx);
	Quaternion yq = Quaternion::fromAxisAngle(0.0, 1.0, 0.0, qy);
	viewq = yq * xq;
}

void update_c_pos() {
	view_position -= viewq * vec4(c_vel_side, 0.0, 0.0, 1.0);
	view_position += viewq * vec4(0.0, 0.0, c_vel_fwd, 1.0);
}

void control()
{
	static const float fwd_modifier = 0.008;
	static const float side_modifier = 0.005;
	static const float mouse_modifier = 0.0004;

	static const float turning_modifier_forward = 0.19;
	static const float turning_modifier_reverse = 0.16;

	static const float accel_modifier = 0.012;
	static const float brake_modifier = 0.010;
	static float dx, dy;
	dx = 0; dy = 0;
	
	if (mouse_locked) {
		GetCursorPos(cursorPos);	// these are screen coordinates, ie. absolute coordinates. it's fine though
		SetCursorPos(HALF_WINDOW_WIDTH, HALF_WINDOW_HEIGHT);
		dx = (HALF_WINDOW_WIDTH - cursorPos->x);
		dy = -(HALF_WINDOW_HEIGHT - cursorPos->y);
	}
	
	if (WM_KEYDOWN_KEYS['W']) { c_vel_fwd += fwd_modifier; }
	if (WM_KEYDOWN_KEYS['S']) { c_vel_fwd -= fwd_modifier; }	
	c_vel_fwd *= 0.96;

	if (WM_KEYDOWN_KEYS['A']) { c_vel_side -= side_modifier; }
	if (WM_KEYDOWN_KEYS['D']) { c_vel_side += side_modifier; }
	c_vel_side *= 0.95;

	if (WM_KEYDOWN_KEYS['N']) {
		WM_KEYDOWN_KEYS['N'] = false;
	}

	if (WM_KEYDOWN_KEYS['P']) {
		PMODE = (PMODE == GL_FILL ? GL_LINE : GL_FILL);
		WM_KEYDOWN_KEYS['P'] = false;
	}

	if (dy != 0) {
		rotatey(mouse_modifier*dy);
	}
	if (dx != 0) {
		rotatex(mouse_modifier*dx);
	}


	// these are active regardless of mouse_locked status
	if (WM_KEYDOWN_KEYS[VK_PRIOR]) {
		onScreenLog::scroll(char_spacing_vert);
		WM_KEYDOWN_KEYS[VK_PRIOR] = false;
	}
	if (WM_KEYDOWN_KEYS[VK_NEXT]) {
		onScreenLog::scroll(-char_spacing_vert);	
		WM_KEYDOWN_KEYS[VK_NEXT] = false;
	}
	if (WM_CHAR_KEYS['L']) {
			onScreenLog::toggle_visibility();
			WM_KEYDOWN_KEYS['L'] = false;
	}
		

}

static GLuint skybox_VBOid;
static GLuint skybox_facecount;
static mat4 skyboxmat = mat4::identity();

inline double rand01() {
	return (double)rand()/(double)RAND_MAX;
}

vec4 randomvec4(float min, float max) {
	float r1, r2, r3, r4;
	r1 = rand01() * max + min;
	r2 = rand01() * max + min;
	r3 = rand01() * max + min;
	r4 = 0;
	vec4 r(r1, r2, r3, r4);
	//r.print();
	return r;
}

GLushort *generateIndices() {
	// due to the design of the .bobj file format, the index buffers just run arithmetically (0, 1, 2, ..., 0xFFFF)
	GLushort index_count = 0xFFFF;
	GLushort *indices = new GLushort[index_count];

	for (int i = 0; i < index_count; i++) {
		indices[i] = i;
	}
	return indices;
}


#define uniform_assert_warn(uniform) do {\
if (uniform == -1) { \
	onScreenLog::print("(warning: uniform optimized away by GLSL compiler: %s at %d:%d\n", #uniform, __FILE__, __LINE__);\
}\
} while(0)

PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

int initGL(void)
{	
	
	if(ogl_LoadFunctions() == ogl_LOAD_FAILED) { 
		return 0;
	}
	glClearColor(0.0, 0.0, 0.0, 1.0);

	onScreenLog::init();
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress ("wglSwapIntervalEXT");  

	glEnable(GL_DEPTH_TEST);

	onScreenLog::print( "OpenGL version: %s\n", glGetString(GL_VERSION));
	onScreenLog::print( "GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	onScreenLog::print( "Loading models...\n");

	chassis.VBOid = loadNewestBObj("models/chassis.bobj", &chassis.facecount);
	wheel.VBOid = loadNewestBObj("models/wheel.bobj", &wheel.facecount);
	plane.VBOid = loadNewestBObj("models/plane.bobj", &plane.facecount);
	onScreenLog::print( "done.\n");
	indices = generateIndices();

	onScreenLog::print( "Loading textures...");
	TextureBank::add(Texture("textures/dina_all.png", GL_NEAREST));
	onScreenLog::print( "done.\n");

	if (!TextureBank::validate()) {
		onScreenLog::print( "Error: failed to validate TextureBank (fatal).\n");
		return 0;
	}

	text_texId = TextureBank::get_id_by_name("textures/dina_all.png");
	
	regular_shader = new ShaderProgram("shaders/regular"); 
	text_shader = new ShaderProgram("shaders/text_shader");

	if (regular_shader->is_bad() || text_shader->is_bad()) { 
		onScreenLog::print( "Error: shader error (fatal).\n");
		return 0; 
	}

	onScreenLog::draw();
	window_swapbuffers();


	glGenBuffers(1, &IBOid);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOid);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short int)*3*facecount, indices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*(GLushort)(0xFFFF), indices, GL_STATIC_DRAW);

	delete [] indices;	
	
	regular_shader->construct_uniform_map();
	text_shader->construct_uniform_map();
	
	glEnableVertexAttribArray(ATTRIB_POSITION);
	glEnableVertexAttribArray(ATTRIB_NORMAL);
	glEnableVertexAttribArray(ATTRIB_TEXCOORD);

	view = mat4::identity();

	projection = mat4::proj_persp(M_PI/8.0, (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 2.0, 1000.0);

	view_position = vec4(0.0, 0.0, -9.0, 1.0);
	cameraVel = vec4(0.0, 0.0, 0.0, 1.0);
		
	glBindBuffer(GL_ARRAY_BUFFER, chassis.VBOid);
	
	glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(0));
	glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(3*sizeof(float)));
	glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(6*sizeof(float)));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOid);

	return 1;

}


void drawCars(const std::unordered_map<unsigned short, struct Peer> &peers) {
	for (auto &iter : peers) {
		const Car &car =  iter.second.car;
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, PMODE);
	glUseProgram(regular_shader->getProgramHandle());	

	running += 0.015;
	if (running > 1000000) { running = 0; }

	regular_shader->update_uniform_1f("running", running);
	regular_shader->update_uniform_mat4("Projection", (const GLfloat*)projection.rawData());

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOid);  // is still in full matafaking effizzect :D 
	glBindBuffer(GL_ARRAY_BUFFER, chassis.VBOid);	 

	glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(0));
	glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(3*sizeof(float)));
	glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(6*sizeof(float)));

	viewq.normalize();
	view = viewq.toRotationMatrix();
	view = view*mat4::translate(view_position);
	
	// no need to reconstruct iterator every time

	static const vec4 wheel_color(0.07, 0.07, 0.07, 1.0);

	mat4 modelview = view * mat4::translate(car.position()) * mat4::rotate(-car.data_symbolic.direction, 0.0, 1.0, 0.0);
	mat4 mw = modelview*mat4::rotate(car.data_symbolic.susp_angle_roll, 1.0, 0.0, 0.0);
	mw *= mat4::rotate(car.data_symbolic.susp_angle_fwd, 0.0, 0.0, 1.0);
	vec4 light_dir = view * vec4(0.0, 1.0, 1.0, 0.0);

	regular_shader->update_uniform_vec4("light_direction", light_dir.rawData());
	regular_shader->update_uniform_mat4("ModelView", mw.rawData());
	regular_shader->update_uniform_vec4("paint_color", colors[iter.second.info.color].rawData());
	glBindBuffer(GL_ARRAY_BUFFER, chassis.VBOid);
	
	// the car doesn't have a texture as of yet :P laterz
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, TEXTURE_ID);
	//regular_shader->update_uniform_1i("texture_color", 0);

	//modelview.print();

	glDisable(GL_BLEND);
	glDrawElements(GL_TRIANGLES, chassis.facecount*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 

	glBindBuffer(GL_ARRAY_BUFFER, wheel.VBOid);
	
	glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(0));
	glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(3*sizeof(float)));
	glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(6*sizeof(float)));
	
	regular_shader->update_uniform_vec4("paint_color", wheel_color.rawData());
	
	// front wheels
	static const mat4 front_left_wheel_translation = mat4::translate(vec4(-2.2, -0.6, 0.9, 1.0));
	mw = modelview;
	mw *= front_left_wheel_translation;
	mw *= mat4::rotate(M_PI - car.data_symbolic.front_wheel_angle, 0.0, 1.0, 0.0);
	mw *= mat4::rotate(-car.data_symbolic.wheel_rot, 0.0, 0.0, 1.0);


	regular_shader->update_uniform_mat4("ModelView", mw.rawData());
	glDrawElements(GL_TRIANGLES, wheel.facecount*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 
	
	static const mat4 front_right_wheel_translation = mat4::translate(vec4(-2.2, -0.6, -0.9, 1.0));
	mw = modelview;
	mw *= front_right_wheel_translation;
	mw *= mat4::rotate(-car.data_symbolic.front_wheel_angle, 0.0, 1.0, 0.0);
	mw *= mat4::rotate(car.data_symbolic.wheel_rot, 0.0, 0.0, 1.0);


	regular_shader->update_uniform_mat4("ModelView", mw.rawData());
	glDrawElements(GL_TRIANGLES, wheel.facecount*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 

	// back wheels
	static const mat4 back_left_wheel_translation = mat4::translate(vec4(1.3, -0.6, 0.9, 1.0));
	mw = modelview;
	mw *= back_left_wheel_translation;
	mw *= mat4::rotate(car.data_symbolic.wheel_rot, 0.0, 0.0, 1.0);
	mw *= mat4::rotate(M_PI, 0.0, 1.0, 0.0);

	
	regular_shader->update_uniform_mat4("ModelView", mw.rawData());
	glDrawElements(GL_TRIANGLES, wheel.facecount*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 
	
	static const mat4 back_right_wheel_translation = mat4::translate(vec4(1.3, -0.6, -0.9, 1.0));

	mw = modelview;
	mw *= back_right_wheel_translation;
	mw *= mat4::rotate(car.data_symbolic.wheel_rot, 0.0, 0.0, 1.0);

	regular_shader->update_uniform_mat4("ModelView", mw.rawData());
	glDrawElements(GL_TRIANGLES, wheel.facecount*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 
}
}


static std::string get_fps(long us_per_frame) {
	double f = 1000000/us_per_frame;
	static char fps[8];
	sprintf_s(fps, 8, "%4.2f", f);
	return std::string(fps);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{	
	/*if(AllocConsole()) {
	// for debugging those early-program fatal erreurz. this will screw up our framerate though.
		freopen("CONOUT$", "wt", stderr);
		SetConsoleTitle("debug output");
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
	}*/
	if(!CreateGLWindow("car XDDDdddd", WINDOW_WIDTH, WINDOW_HEIGHT, 32, FALSE)) { return 1; }
	if (!initGL()) { return 1; }
	
	wglSwapIntervalEXT(1);

	std::string cpustr(checkCPUCapabilities());
	if (cpustr.find("ERROR") != std::string::npos) { MessageBox(NULL, cpustr.c_str(), "Fatal error.", MB_OK); return -1; }
	
	onScreenLog::print("%s\n", cpustr.c_str());
	
	MSG msg;
	BOOL done=FALSE;

	std::ifstream ip_file("REMOTE_IP");
	std::string remote_ip;

	if (!ip_file.is_open()) {
		remote_ip = "127.0.0.1";
	}
	else {
		std::getline(ip_file, remote_ip);
	}

	LocalClient::init("Jarmo2", remote_ip, (unsigned short)50000);
	
	_timer timer;
	
	while(!done)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
		{
			if(msg.message == WM_QUIT)
			{
				onScreenLog::print("Sending quit message (C_QUIT) to server.\n");
				LocalClient::quit();
				
				done=TRUE;
			}
			else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		
		if (WM_KEYDOWN_KEYS[VK_ESCAPE])
		{
			ShowCursor(mouse_locked);
			mouse_locked = !mouse_locked;

			if (!mouse_locked) { 
				set_cursor_relative_pos(HALF_WINDOW_WIDTH, HALF_WINDOW_HEIGHT);
			}
			WM_KEYDOWN_KEYS[VK_ESCAPE] = FALSE;
		}
		control();
		update_c_pos();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawCars(LocalClient::get_peers());
		onScreenLog::draw();
		window_swapbuffers();

		onScreenLog::dispatch_print_queue();
		//long us_per_frame = timer.get_us();
	
		//timer.begin();
			
		
	}


	return (msg.wParam);
}


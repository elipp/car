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

float height_sample_above_camera = 0;

struct meshinfo {
	GLuint VBOid;
	GLuint facecount;
};

static Model *chassis = NULL, 
			 *wheel = NULL, 
			 *racetrack = NULL,
			 *terrain = NULL;

HeightMap *map = NULL;

GLuint IBOid, FBOid, FBO_textureId;

bool mouseLocked = false;

GLfloat running = 0.0;

static ShaderProgram *regular_shader = NULL,
					 *normal_plot_shader = NULL,
					 *racetrack_shader = NULL;

mat4 view;
Quaternion viewq;
static float qx = 0, qy = 0;
mat4 projection;
vec4 view_position;
vec4 cameraVel;


GLuint road_texId;
GLuint terrain_texId;

#ifndef M_PI
#define M_PI 3.1415926535
#endif

float PROJ_FOV_RADIANS = (M_PI/4);
float PROJ_Z_FAR = 800.0;

static int vsync = 1;

static float height_sample_under_car = 0.0;

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
	viewq.normalize();
	view = viewq.toRotationMatrix();
	view = view*mat4::translate(view_position);
}

void control()
{
	static const float fwd_modifier = 0.012;
	static const float side_modifier = 0.012;
	static const float mouse_modifier = 0.0006;

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

	if (WM_KEYDOWN_KEYS['V']) {
		vsync ^= 1;
		wglSwapIntervalEXT(vsync);
		onScreenLog::print("vsync: %d\n", vsync);
		WM_KEYDOWN_KEYS['V'] = false;
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
	// this means the maximum amount of (triangle) faces per VBO equals 0xFFFF/3 = 21845
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
	ResizeGLScene(WINDOW_WIDTH, WINDOW_HEIGHT);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	
	onScreenLog::init();
	VarTracker::init();
	
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress ("wglSwapIntervalEXT");  

	glEnable(GL_DEPTH_TEST);
	
	onScreenLog::print( "OpenGL version: %s\n", glGetString(GL_VERSION));
	onScreenLog::print( "GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
		
	regular_shader = new ShaderProgram("shaders/regular"); 
	text_shader = new ShaderProgram("shaders/text_shader");
	racetrack_shader = new ShaderProgram("shaders/racetrack");

	if (regular_shader->is_bad() ||
		text_shader->is_bad() ||
		racetrack_shader->is_bad()) { 

		messagebox_error("Error: shader error. See shader.log.\n");
		return 0; 
	}
	
	onScreenLog::print( "Loading models...\n");
	
	chassis = new Model("models/chassis.bobj", regular_shader);
	wheel = new Model("models/wheel.bobj", regular_shader);
	racetrack = new Model("models/racetrack.bobj", racetrack_shader);
	terrain = new Model("models/mappi.bobj", racetrack_shader);

	if (chassis->bad() || wheel->bad() || racetrack->bad() || terrain->bad()) {
		messagebox_error("initGL error: model load error.\n");
		return 0; 
	}

	onScreenLog::print( "done.\n");
	
	GLushort *indices = generateIndices();
	
	onScreenLog::print( "Loading textures...");
	text_texId = TextureBank::add(Texture("textures/dina_all.png", GL_NEAREST));
	road_texId = TextureBank::add(Texture("textures/road.jpg", GL_LINEAR_MIPMAP_LINEAR));
	terrain_texId = TextureBank::add(Texture("textures/grass.jpg", GL_LINEAR_MIPMAP_LINEAR));
	onScreenLog::print( "done.\n");
	
	if (!TextureBank::validate()) {
		messagebox_error("Error: failed to validate TextureBank (fatal).\n");
		return 0;
	}

	map = new HeightMap("textures/heightmap_grayscale.jpg", 512);
	if (map->bad()) {
		onScreenLog::print("heightmap failure. .\n");
	}

	racetrack->bind_texture(road_texId);
	terrain->bind_texture(terrain_texId);

	onScreenLog::draw();
	window_swapbuffers();

	glGenBuffers(1, &IBOid);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOid);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*(int)(0xFFFF), indices, GL_STATIC_DRAW);

	delete [] indices;	
	
	glEnableVertexAttribArray(ATTRIB_POSITION);
	glEnableVertexAttribArray(ATTRIB_NORMAL);
	glEnableVertexAttribArray(ATTRIB_TEXCOORD);

	view = mat4::identity();

	projection = mat4::proj_persp(PROJ_FOV_RADIANS, (WINDOW_WIDTH/WINDOW_HEIGHT), 4.0, PROJ_Z_FAR);

	view_position = vec4(0.0, -150, 0.0, 1.0); // displacement would be a better name
	cameraVel = vec4(0.0, 0.0, 0.0, 1.0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOid);
	
	wglSwapIntervalEXT(vsync);

	VarTracker_track(vec4, view_position);

	return 1;

}


void drawRacetrack() {
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, PMODE);
	
	vec4 light_dir = view * vec4(0.0, 1.0, 1.0, 0.0);

	racetrack_shader->update_uniform_vec4("light_direction", light_dir);

	static const mat4 racetrack_model = mat4::translate(5.0, 0.0, 0.0)*mat4::scale(32,32,32) * mat4::rotate(PI_PER_TWO, 1.0, 0.0, 0.0);

	mat4 racetrack_modelview = view * racetrack_model;
	racetrack->use_ModelView(racetrack_modelview);
	
	racetrack->draw();
}

void drawTerrain() {
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, PMODE);
	
	vec4 light_dir = view * vec4(0.0, 1.0, 1.0, 0.0);
	racetrack_shader->update_uniform_vec4("light_direction", light_dir);

	static const mat4 racetrack_model = mat4::scale(32, 32, 32) * mat4::rotate(PI_PER_TWO, 1.0, 0.0, 0.0);

	mat4 terrain_modelview = view * racetrack_model;
	terrain->use_ModelView(terrain_modelview);
	
	terrain->draw();
}

void drawCars(const std::unordered_map<unsigned short, struct Peer> &peers) {
	for (auto &iter : peers) {
		const Car &car =  iter.second.car;
		glEnable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, PMODE);

		//running += 0.015;
		//if (running > 1000000) { running = 0; }

		//regular_shader->update_uniform_1f("running", running);
		regular_shader->update_uniform_mat4("Projection", projection);

		static const vec4 wheel_color(0.07, 0.07, 0.07, 1.0);

		vec4 test_pos = car.position();
		height_sample_under_car =  map->lookup(test_pos(V::x), test_pos(V::z));
		test_pos(V::y) = height_sample_under_car - 150;

		mat4 modelview = view * mat4::translate(test_pos) * mat4::rotate(-car.state.direction, 0.0, 1.0, 0.0);
		mat4 mw = modelview*mat4::rotate(car.state.susp_angle_roll, 1.0, 0.0, 0.0);
		//mw *= mat4::rotate(car.state.susp_angle_fwd, 0.0, 0.0, 1.0);
		vec4 light_dir = view * vec4(0.0, 1.0, 1.0, 0.0);

		regular_shader->update_uniform_vec4("light_direction", light_dir);
		chassis->use_ModelView(mw);
		regular_shader->update_uniform_vec4("paint_color", colors[iter.second.info.color]);

		chassis->draw();
	
		regular_shader->update_uniform_vec4("paint_color", wheel_color);
	
		// front wheels
		static const mat4 front_left_wheel_translation = mat4::translate(vec4(-2.2, -0.6, 0.9, 1.0));
		mw = modelview;
		mw *= front_left_wheel_translation;
		mw *= mat4::rotate(M_PI - car.state.front_wheel_angle, 0.0, 1.0, 0.0);
		mw *= mat4::rotate(-car.state.wheel_rot, 0.0, 0.0, 1.0);

		wheel->use_ModelView(mw);
		wheel->draw();

		static const mat4 front_right_wheel_translation = mat4::translate(vec4(-2.2, -0.6, -0.9, 1.0));
		mw = modelview;
		mw *= front_right_wheel_translation;
		mw *= mat4::rotate(-car.state.front_wheel_angle, 0.0, 1.0, 0.0);
		mw *= mat4::rotate(car.state.wheel_rot, 0.0, 0.0, 1.0);

		wheel->use_ModelView(mw);
		wheel->draw();
	
		// back wheels
		static const mat4 back_left_wheel_translation = mat4::translate(vec4(1.3, -0.6, 0.9, 1.0));
		mw = modelview;
		mw *= back_left_wheel_translation;
		mw *= mat4::rotate(car.state.wheel_rot, 0.0, 0.0, 1.0);
		mw *= mat4::rotate(M_PI, 0.0, 1.0, 0.0);

		wheel->use_ModelView(mw);
		wheel->draw();
	
		static const mat4 back_right_wheel_translation = mat4::translate(vec4(1.3, -0.6, -0.9, 1.0));

		mw = modelview;
		mw *= back_right_wheel_translation;
		mw *= mat4::rotate(car.state.wheel_rot, 0.0, 0.0, 1.0);
		
		wheel->use_ModelView(mw);
		wheel->draw();
	}
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{	
	
	/* if(AllocConsole()) {
	// for debugging those early-program fatal erreurz. this will screw up our framerate though.
		freopen("CONOUT$", "wt", stderr);
		SetConsoleTitle("debug output");
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
	} */

	if (!CreateGLWindow("car XDDDdddd", WINDOW_WIDTH, WINDOW_HEIGHT, 32, FALSE)) { return 1; }
	if (!initGL()) {
		return 1; 
	}
	
	int k = timeBeginPeriod(1);
	long wait = 0;
	double time_per_frame_ms = 0;

	VarTracker_track(double, time_per_frame_ms);
	VarTracker_track(float, height_sample_under_car);

	onScreenLog::print("timeBeginPeriod(1) returned %s.\n", k == TIMERR_NOERROR ? "TIMERR_NOERROR (good!)" : "TIMERR_NOCANDO (bad.)" );
	
	std::string cpustr(checkCPUCapabilities());
	if (cpustr.find("ERROR") != std::string::npos) { MessageBox(NULL, cpustr.c_str(), "Fatal error.", MB_OK); return -1; }
	
	onScreenLog::print("%s\n", cpustr.c_str());
	
	LocalClient::set_name("Player");
	onScreenLog::print("\nUse /connect <ip>:<port> to connect to a server, or /startserver to start one.\n");
	onScreenLog::print("(/connect without an argument will connect to localhost:50000.)\n");
	
	_timer fps_timer;
	fps_timer.begin();
	
	static float running = 0;
	MSG msg;
	
	while(main_loop_running())
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
		{
			if(msg.message == WM_QUIT)
			{
				onScreenLog::print("Sending quit message (C_QUIT) to server.\n");
				LocalClient::quit();
				stop_main_loop();
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

		if (LocalClient::shutdown_requested()) { // this flag is set by a number of conditions in the client code
			// stop must be explicitly called from a thread that's not involved with all the net action (ie. this one)
			LocalClient::stop();
		}
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		control();
		update_c_pos();

		drawTerrain();
		onScreenLog::draw();
		VarTracker::draw();

		onScreenLog::dispatch_print_queue();
		onScreenLog::input_field.refresh();
		
		if (LocalClient::connected()) {
			//double t = LocalClient::time_since_last_posupd_ms();
			LocalClient::interpolate_positions();
			drawCars(LocalClient::get_peers());
			
		}

		wait = 16.666 - fps_timer.get_ms();

		if (!vsync) { // then we'll do a "half-busy" wait :P
			if (wait > 3) { Sleep(wait-1); }	// sleep for those 4 milliseconds
			while(fps_timer.get_ms() < 16.6);	// this is accurate, ie. busy wait
		}

		window_swapbuffers();
		time_per_frame_ms = fps_timer.get_ms();

		fps_timer.begin();
			
		
	}
	timeEndPeriod(1);


	return (msg.wParam);
}


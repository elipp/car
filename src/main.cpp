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

#include "physics/OBB.h"

#include "lin_alg.h"
#include "common.h"
#include "objloader.h"
#include "shader.h"
#include "texture.h"
#include "text.h"
#include "vertex.h"

static GLint PMODE = GL_FILL;	// polygon mode toggle
static LPPOINT cursorPos = new POINT;	/* holds cursor position */

static const float map_scale = 32;

extern bool active;

float c_vel_fwd = 0, c_vel_side = 0;

float height_sample_above_camera = 0;

static Model *chassis = NULL, 
			 *wheel = NULL, 
			 *racetrack = NULL,
			 *terrain = NULL,
			 *skybox = NULL,
			 *cube = NULL;

HeightMap *map = NULL;

GLuint IBOid, FBOid, FBO_textureId;

bool mouseLocked = false;

GLfloat running = 0.0;

static ShaderProgram *regular_shader = NULL,
					 *normal_plot_shader = NULL,
					 *racetrack_shader = NULL,
					 *skybox_shader = NULL;
mat4 view;
Quaternion viewq;
mat4 projection;
vec4 view_position;
vec4 cameraVel;

static GLuint minkowski_VBOid;
static GLuint minkowski_IBOid;

static vec4 camera_position;

GLuint road_texId;
GLuint terrain_texId;
GLuint skybox_texId;

#ifndef M_PI
#define M_PI 3.1415926535
#endif

float PROJ_FOV_RADIANS = (M_PI/4);
float PROJ_Z_FAR = 2000.0;

static int vsync = 1;

static float height_sample_under_car = 0.0;
static vec4 car_pos;	
static vec4 penetration_depth;

static OBB OBBa, OBBb;
static Quaternion OBBaQ, OBBbQ;

static bool spam_test = false;

void handle_WM_KEYDOWN(WPARAM wParam) {

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
}

void handle_WM_CHAR(WPARAM wParam) {
	if (onScreenLog::input_field.enabled()) {
		if (wParam != VK_RETURN) {
			onScreenLog::input_field.insert_char_to_cursor_pos(wParam);
		}
	}
}
void rotateview(float modx, float mody) {
	static float qx = 0;
	static float qy = 0;
	qx += modx;
	qy -= mody;
	Quaternion xq = Quaternion::fromAxisAngle(1.0, 0.0, 0.0, qy);
	Quaternion yq = Quaternion::fromAxisAngle(0.0, 1.0, 0.0, qx);
	viewq = yq*xq;
}

void update_c_pos() {
	view_position -= vec4(c_vel_side, 0.0, 0.0, 1.0).applyQuatRotation(viewq);
	view_position += vec4(0.0, 0.0, c_vel_fwd, 1.0).applyQuatRotation(viewq);
	viewq.normalize();
	view = viewq.toRotationMatrix();
	view = view*mat4::translate(view_position);
}

void control()
{
	static const float fwd_modifier = 0.024;
	static const float side_modifier = 0.020;
	static const float mouse_modifier = 0.0006;

	static float dx, dy;
	dx = 0; dy = 0;
	
	if (mouse_locked) {
		GetCursorPos(cursorPos);	// these are screen coordinates, ie. absolute coordinates. it's fine though
		SetCursorPos(HALF_WINDOW_WIDTH, HALF_WINDOW_HEIGHT);
		dx = ((LONG)HALF_WINDOW_WIDTH - cursorPos->x);
		dy = -((LONG)HALF_WINDOW_HEIGHT - cursorPos->y);
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
		PRINT("vsync: %d\n", vsync);
		WM_KEYDOWN_KEYS['V'] = false;
	}

	rotateview(mouse_modifier*dx, mouse_modifier*dy);

	// these are active regardless of mouse_locked status
	if (WM_KEYDOWN_KEYS[VK_PRIOR]) {
		onScreenLog::scroll(1);
		WM_KEYDOWN_KEYS[VK_PRIOR] = false;
	}
	if (WM_KEYDOWN_KEYS[VK_NEXT]) {
		onScreenLog::scroll(-1);	
		WM_KEYDOWN_KEYS[VK_NEXT] = false;
	}
	if (WM_CHAR_KEYS['L']) {
			onScreenLog::toggle_visibility();
			WM_KEYDOWN_KEYS['L'] = false;
	}

	if (WM_KEYDOWN_KEYS['O']) {
		OBBa.C.assign(V::x, OBBa.C(V::x) + 0.03);
	}
	if (WM_KEYDOWN_KEYS['I']) {
		OBBa.C.assign(V::x, OBBa.C(V::x) - 0.03);
	}
	if (WM_KEYDOWN_KEYS['K']) { 
		OBBa.C.assign(V::y, OBBa.C(V::y) + 0.03);
	}
	if (WM_KEYDOWN_KEYS['J']) {
		OBBa.C.assign(V::y, OBBa.C(V::y) - 0.03);
	}
	if (WM_KEYDOWN_KEYS['R']) {
		OBBaQ = OBBaQ*Quaternion::fromAxisAngle(0.0, 1.0, 0.0, 0.03);
		OBBaQ.normalize();
	}
	if (WM_KEYDOWN_KEYS['T']) {
		OBBaQ = OBBaQ*Quaternion::fromAxisAngle(1.0, 0.0, 0.0, 0.03);
		OBBaQ.normalize();
	}
	if (WM_KEYDOWN_KEYS['Y']) {
		OBBb.C.assign(V::z, OBBb.C(V::z) + 0.03);
	}
	if (WM_KEYDOWN_KEYS['U']) {
		OBBb.C.assign(V::z, OBBb.C(V::z) - 0.03);
	}
	
	if (WM_KEYDOWN_KEYS['F']) {
		OBBbQ = OBBbQ*Quaternion::fromAxisAngle(0.0, 0.0, 1.0, 0.03);
	}

	if (WM_KEYDOWN_KEYS['0']) {
		spam_test = !spam_test;
		WM_KEYDOWN_KEYS['0'] = FALSE;
	}

	camera_position = -view_position;
	camera_position.assign(V::z, camera_position(V::z)*(-1));

}

void resize_GL_scene(GLsizei width, GLsizei height)	{
	height = MAXIMUM(height, 1);

	WINDOW_WIDTH = width;
	WINDOW_HEIGHT = height;

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);	

	text_set_Projection(mat4::proj_ortho(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0, -1.0, 1.0));
	projection = mat4::proj_persp(PROJ_FOV_RADIANS, ((float)WINDOW_WIDTH/(float)WINDOW_HEIGHT), 4.0, PROJ_Z_FAR);
	onScreenLog::update_position();
	onScreenLog::input_field.update_position();
	VarTracker::update_position();
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

static void init_minkowski_buffers() {
	GLushort minkowski_indices[64];
	for (GLushort i = 0; i < 64; ++i) {
		minkowski_indices[i] = i;
	}	
		
	glGenBuffers(1, &minkowski_VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, minkowski_VBOid);
	glBufferData(GL_ARRAY_BUFFER, 64*sizeof(vertex), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &minkowski_IBOid);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, minkowski_IBOid);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 64*sizeof(GLushort), minkowski_indices, GL_STATIC_DRAW);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

}


#define uniform_assert_warn(uniform) do {\
if (uniform == -1) { \
	PRINT("(warning: uniform optimized away by GLSL compiler: %s at %d:%d\n", #uniform, __FILE__, __LINE__);\
}\
} while(0)

int initGL(void)
{	

	resize_GL_scene(WINDOW_WIDTH, WINDOW_HEIGHT);
	
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);	
	
#define ADD_ATTRIB(map, attrib_id, attrib_name) do {\
	(map).insert(std::make_pair<GLuint, std::string>((attrib_id), std::string(attrib_name)));\
	} while(0)

	std::unordered_map<GLuint, std::string> text_attrib_bindings;
	ADD_ATTRIB(text_attrib_bindings, TEXT_ATTRIB_ALL, "ALL_ATTRIBS_PACKED");

	text_shader = new ShaderProgram("shaders/text_shader", text_attrib_bindings);
	text_texId = TextureBank::add(Texture("textures/dina_all.png", GL_NEAREST));
	
	std::unordered_map<GLuint, std::string> overlay_attrib_bindings;
	ADD_ATTRIB(overlay_attrib_bindings, OVERLAY_ATTRIB_POS, "attribs_packed");
	overlay_shader = new ShaderProgram("shaders/overlay", overlay_attrib_bindings);

	onScreenLog::init();
	VarTracker::init();
	
	glEnable(GL_DEPTH_TEST);

	PRINT("OpenGL version: %s\n", glGetString(GL_VERSION));
	PRINT("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	
	std::unordered_map<GLuint, std::string> default_attrib_bindings;
	ADD_ATTRIB(default_attrib_bindings, ATTRIB_POSITION, "Position_VS_in");
	ADD_ATTRIB(default_attrib_bindings, ATTRIB_NORMAL, "Normal_VS_in");
	ADD_ATTRIB(default_attrib_bindings, ATTRIB_TEXCOORD, "TexCoord_VS_in");
	
	regular_shader = new ShaderProgram("shaders/regular", default_attrib_bindings); 
	racetrack_shader = new ShaderProgram("shaders/racetrack", default_attrib_bindings);
	skybox_shader = new ShaderProgram("shaders/skybox", default_attrib_bindings);


	if (regular_shader->is_bad() ||
		text_shader->is_bad() ||
		racetrack_shader->is_bad() ||
		skybox_shader->is_bad() ||
		overlay_shader->is_bad()) { 

		messagebox_error("Error: shader error. See shader.log.\n");
		return 0; 
	}
	
	PRINT("Loading stuff...\n");

	onScreenLog::draw();
	window_swapbuffers();
	
	chassis = new Model("models/chassis.bobj", regular_shader);
	wheel = new Model("models/wheel.bobj", regular_shader);
	terrain = new Model("models/mappi.bobj", racetrack_shader);
	skybox = new Model("models/skybox.bobj", skybox_shader);
	cube = new Model("models/cube.bobj", regular_shader);

	if (chassis->bad() || wheel->bad() || terrain->bad() || skybox->bad() || cube->bad()) {
		messagebox_error("initGL error: model load error.\n");
		return 0; 
	}

	PRINT("done.\n");
	
	PRINT("Loading textures...");
	terrain_texId = TextureBank::add(Texture("textures/grass.jpg", GL_LINEAR_MIPMAP_LINEAR));
	skybox_texId = TextureBank::add(Texture("textures/skybox.jpg", GL_NEAREST));
	PRINT("done.\n");
	
	if (!TextureBank::validate()) {
		messagebox_error("Error: failed to validate TextureBank (fatal).\n");
		return 0;
	}

	map = new HeightMap("textures/heightmap_grayscale.png", map_scale, 2.7475, -2.7475); // the top/bottom values were calculated by blender
	if (map->bad()) {
		PRINT("heightmap failure. .\n");
	}


	terrain->bind_texture(terrain_texId);
	skybox->bind_texture(skybox_texId);

	view = mat4::identity();

	projection = mat4::proj_persp(PROJ_FOV_RADIANS, (WINDOW_WIDTH/WINDOW_HEIGHT), 4.0, PROJ_Z_FAR);

	view_position = vec4(0.0, -45, 0.0, 1.0); // displacement would be a better name
	cameraVel = vec4(0.0, 0.0, 0.0, 1.0);
	
	init_minkowski_buffers();

	OBBa = OBB(vec4(1.0, 1.0, 1.0, 0.0));
	OBBb = OBB(vec4(1.0, 1.0, 1.0, 0.0));

	return TRUE;
}




static vertex vec4_to_vertex(const vec4 &v) {
	vertex r;
	r.vx = v(V::x);
	r.vy = v(V::y);
	r.vz = v(V::z);
	// we don't care about normalz or texcoords
	return r;
}

void drawCubes() {	
	GJKSession GJKsess(OBBa, OBBb);
	int are_intersecting = GJKsess.collision_test();
	penetration_depth = vec4(0, 0, 0, 0);
	if (are_intersecting) {
		int r = GJKsess.EPA_penetration(&penetration_depth);
		if (r != EPA_SUCCESS) {
			// anal gay :D
		}
	}
	
	OBBa.rotate(OBBaQ);
	OBBb.rotate(OBBbQ);

	vec4 light_dir = view * vec4(0.0, 1.0, 1.0, 0.0);
	regular_shader->update_uniform_vec4("light_direction", light_dir);
	cube->use_ModelView(view*mat4::translate(OBBa.C)*OBBaQ.toRotationMatrix());
	regular_shader->update_uniform_vec4("paint_color", are_intersecting ? vec4(0.7, 0.2, 0.3, 0.6) :  vec4(0.2, 0.7, 0.3, 1.0));
	cube->draw();
	
	cube->use_ModelView(view*mat4::translate(OBBb.C)*OBBbQ.toRotationMatrix());
	cube->draw();

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

	static const mat4 racetrack_model = mat4::scale(map_scale, map_scale, map_scale);

	mat4 terrain_modelview = view * racetrack_model;
	terrain->use_ModelView(terrain_modelview);
	
	terrain->draw();
}

void drawSkybox() {
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, PMODE);

	static const mat4 skybox_model = mat4::scale(250, 250, 250);

	mat4 skybox_modelview = view * skybox_model;
	skybox->use_ModelView(skybox_modelview);

	skybox->draw();
	glClear(GL_DEPTH_BUFFER_BIT);	// this is really important ^^
	glEnable(GL_DEPTH_TEST);
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

		car_pos = car.position();
		vec4 test_pos = car.position();
		height_sample_under_car = map->lookup(test_pos(V::x), -test_pos(V::z));
		test_pos.assign(V::y, height_sample_under_car+0.90);

		//vec4 test_pos_neg = -test_pos;
		
		//view = mat4::translate(0.0, 0.0, -12.0);
		//view *= mat4::rotate(M_PI/8, -1.0, 0.0, 0.0);
		//view *= mat4::rotate(car.state.direction + PI_PER_TWO, 0.0, 1.0, 0.0);
		//view *= (test_pos_neg).toTranslationMatrix();


		mat4 modelview = view * mat4::translate(test_pos) * mat4::rotate(-car.state.direction, 0.0, 1.0, 0.0);
		mat4 mv = modelview*mat4::rotate(car.state.susp_angle_roll, 1.0, 0.0, 0.0);
		//mv *= mat4::rotate(car.state.susp_angle_fwd, 0.0, 0.0, 1.0);
		vec4 light_dir = view * vec4(0.0, 1.0, 1.0, 0.0);

		regular_shader->update_uniform_vec4("light_direction", light_dir);
		chassis->use_ModelView(mv);
		regular_shader->update_uniform_vec4("paint_color", colors[iter.second.info.color]);

		chassis->draw();
	
		regular_shader->update_uniform_vec4("paint_color", wheel_color);
	
		// front wheels
		static const mat4 front_left_wheel_translation = mat4::translate(-2.2, -0.6, 0.9);
		mv = modelview;
		mv *= front_left_wheel_translation;
		mv *= mat4::rotate(M_PI - car.state.front_wheel_angle, 0.0, 1.0, 0.0);
		mv *= mat4::rotate(-car.state.wheel_rot, 0.0, 0.0, 1.0);

		wheel->use_ModelView(mv);
		wheel->draw();

		static const mat4 front_right_wheel_translation = mat4::translate(-2.2, -0.6, -0.9);
		mv = modelview;
		mv *= front_right_wheel_translation;
		mv *= mat4::rotate(-car.state.front_wheel_angle, 0.0, 1.0, 0.0);
		mv *= mat4::rotate(car.state.wheel_rot, 0.0, 0.0, 1.0);

		wheel->use_ModelView(mv);
		wheel->draw();
	
		// back wheels
		static const mat4 back_left_wheel_translation = mat4::translate(1.3, -0.6, 0.9);
		mv = modelview;
		mv *= back_left_wheel_translation;
		mv *= mat4::rotate(car.state.wheel_rot, 0.0, 0.0, 1.0);
		mv *= mat4::rotate(M_PI, 0.0, 1.0, 0.0);

		wheel->use_ModelView(mv);
		wheel->draw();
	
		static const mat4 back_right_wheel_translation = mat4::translate(1.3, -0.6, -0.9);

		mv = modelview;
		mv *= back_right_wheel_translation;
		mv *= mat4::rotate(car.state.wheel_rot, 0.0, 0.0, 1.0);
		
		wheel->use_ModelView(mv);
		wheel->draw();
	}
}

static void draw_text() {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	draw_overlays(vec4(0.05, 0.05, 0.05, 0.76));
//	glClear(GL_DEPTH_BUFFER_BIT);
	onScreenLog::draw();
	VarTracker::draw();
	glDisable(GL_BLEND);

}

static void draw_everything() {


	drawSkybox();	
	glEnable(GL_DEPTH_TEST);
	drawCubes();

	if (LocalClient::connected()) {
		double t = LocalClient::time_since_last_posupd_ms();
		LocalClient::interpolate_positions();
		drawCars(LocalClient::get_peers());
	} 

	drawTerrain();	
	draw_text();

}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{	
#ifdef ENABLE_CONSOLE
	if(AllocConsole()) {
	 // for debugging those early-program fatal erreurz. this will screw up our framerate though.
		FILE *dummy;
		freopen_s(&dummy, "CONOUT$", "wt", stderr);

		SetConsoleTitle("debug output");
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
	} 
#endif
	if (!CreateGLWindow("car XDDDdddd", WINDOW_WIDTH, WINDOW_HEIGHT, 32, FALSE, hInstance, nCmdShow)) { return 1; }
	if (!initGL()) {
		return 1; 
	}
	
	int k = timeBeginPeriod(1);
	
	PRINT("timeBeginPeriod(1) returned %s\n", k == TIMERR_NOERROR ? "TIMERR_NOERROR (good!)" : "TIMERR_NOCANDO (bad.)" );

	long wait = 0;
	static double time_per_frame_ms = 0;

	VarTracker_track(double, time_per_frame_ms);
	VarTracker_track(float, height_sample_under_car);
	VarTracker_track(vec4, camera_position);
	VarTracker_track(vec4, car_pos);
	VarTracker_track(vec4, penetration_depth);
	//VarTracker_track(mat4, view);

	
	std::string cpustr(checkCPUCapabilities());
	if (cpustr.find("ERROR") != std::string::npos) { MessageBox(NULL, cpustr.c_str(), "Fatal error.", MB_OK); return -1; }
	
	PRINT("%s\n", cpustr.c_str());
	
	char name_buf[UNLEN+1];
	DWORD len = UNLEN+1;
	GetUserName(name_buf, &len);

	LocalClient::set_name(std::string(name_buf));
	PRINT("\nUse /connect <ip>:<port> to connect to a server, or /startserver to start one.\n");
	PRINT("(/connect without an argument will connect to localhost:50000.)\n");
	
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
				PRINT("Sending quit message (C_QUIT) to server.\n");
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

		control();
		update_c_pos();
		if (spam_test) {
			static int spamcounter = 0;
			PRINT("SPAM MESSAGE #%d!!!!\na\nb\nc\nd\ne\nf\ng\nh\ni\nj\nk\nl\nm\no", spamcounter);
			++spamcounter;
		}
		
		onScreenLog::dispatch_print_queue();
		onScreenLog::input_field.refresh();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		draw_everything();

		wait = 16.666 - fps_timer.get_ms();

		if (!vsync) { // then we'll do a "half-busy" wait :P
			if (wait > 3) { Sleep((DWORD)(wait-1)); }
			while(fps_timer.get_ms() < 16.7);	// this is the (more) accurate, ie. busy, part
		}
		
		time_per_frame_ms = fps_timer.get_ms();
		window_swapbuffers(); // calls wglSwapBuffers on the current context. if vsync is enabled, the current thread is blocked

		fps_timer.begin();
		
	}
	timeEndPeriod(1);


	return (msg.wParam);
}


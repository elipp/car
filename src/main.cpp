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

#include "net/net_server.h"
#include "net/net_client.h"

#include "lin_alg.h"
#include "common.h"
#include "objloader.h"
#include "shader.h"
#include "model.h"
#include "texture.h"
#include "text.h"
#include "vertex.h"



static GLint PMODE = GL_FILL;	// polygon mode toggle
static LPPOINT cursorPos = new POINT;	/* holds cursor position */

extern bool active;
extern bool keys[];

static const double GAMMA = 6.67;

float c_vel_fwd = 0, c_vel_side = 0;

struct mesh {
	GLuint VBOid;
	GLuint facecount;
};

struct mesh chassis, wheel, plane;

GLuint IBOid, FBOid, FBO_textureId;

GLfloat tess_level_inner = 1.0;
GLfloat tess_level_outer = 1.0;

static GLint earth_tex_id, hmap_id;

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

static std::vector<Model> models;

static Car local_car;

GLushort * indices; 

#ifndef M_PI
#define M_PI 3.1415926535
#endif

static const float FW_ANGLE_LIMIT = M_PI/6; // rad
static const float SQRT_FW_ANGLE_LIMIT_RECIP = 1.0/sqrt(FW_ANGLE_LIMIT);
// f(x) = -(1/(x+1/sqrt(30))^2) + 30
// f(x) = 1/(-x + 1/sqrt(30))^2 - 30

float f_wheel_angle(float x) {
	float t;
	if (x >= 0) {
		t = x+SQRT_FW_ANGLE_LIMIT_RECIP;
		t *= t;
		return -(1/t) + FW_ANGLE_LIMIT;
	}
	else {
		t = -x+SQRT_FW_ANGLE_LIMIT_RECIP;
		t *= t;
		return (1/t) - FW_ANGLE_LIMIT;
	}
}

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

	if(mouseLocked) {
		unsigned int buttonmask;
		
		GetCursorPos(cursorPos);
		SetCursorPos(HALF_WINDOW_WIDTH, HALF_WINDOW_HEIGHT);
		float dx = (HALF_WINDOW_WIDTH - cursorPos->x);
		float dy = -(HALF_WINDOW_HEIGHT - cursorPos->y);

		static float local_car_acceleration = 0.0;
		static float local_car_prev_velocity;
		local_car_prev_velocity = local_car.velocity;

		if (keys[VK_UP]) {
			local_car.velocity += accel_modifier;
		}
		else {
			local_car.susp_angle_roll *= 0.90;
			local_car.front_wheel_tmpx *= 0.50;
		}
		
		if (keys[VK_DOWN]) {
			if (local_car.velocity > 0.01) {
				local_car.direction -= 3*local_car.F_centripetal*local_car.velocity*0.20;
				local_car.velocity *= 0.99;
			}
			local_car.velocity -= brake_modifier;

		}
		else {
				local_car.susp_angle_roll *= 0.90;
				local_car.front_wheel_tmpx *= 0.50;
		}
		local_car.velocity *= 0.95;	// regardless of keystate

		if (keys[VK_LEFT]) {
			if (local_car.front_wheel_tmpx < 15.0) {
				local_car.front_wheel_tmpx += 0.5;
			}
			local_car.F_centripetal = -1.0;
			local_car.front_wheel_angle = f_wheel_angle(local_car.front_wheel_tmpx);
			local_car.susp_angle_roll = fabs(local_car.front_wheel_angle*local_car.velocity*0.8);
			if (local_car.velocity > 0) {
				local_car.direction -= turning_modifier_forward*local_car.F_centripetal*local_car.velocity;
			}
			else {
				local_car.direction -= turning_modifier_reverse*local_car.F_centripetal*local_car.velocity;
			}

		} 
		if (keys[VK_RIGHT]) {
			if (local_car.front_wheel_tmpx > -15.0) {
				local_car.front_wheel_tmpx -= 0.5;
			}
			local_car.F_centripetal = 1.0;
			local_car.front_wheel_angle = f_wheel_angle(local_car.front_wheel_tmpx);
			local_car.susp_angle_roll = -fabs(local_car.front_wheel_angle*local_car.velocity*0.8);
			if (local_car.velocity > 0) {
				local_car.direction -= turning_modifier_forward*local_car.F_centripetal*local_car.velocity;
			}
			else {
				local_car.direction -= turning_modifier_reverse*local_car.F_centripetal*local_car.velocity;
			}

		} 

		local_car_prev_velocity = 0.5*(local_car.velocity+local_car_prev_velocity);
		local_car_acceleration = 0.2*(local_car.velocity - local_car_prev_velocity) + 0.8*local_car_acceleration;

	if (!keys[VK_LEFT] && !keys[VK_RIGHT]){
		local_car.front_wheel_tmpx *= 0.30;
		local_car.front_wheel_angle = f_wheel_angle(local_car.front_wheel_tmpx);
		local_car.susp_angle_roll *= 0.50;
		local_car.susp_angle_fwd *= 0.50;
		local_car.F_centripetal = 0.0;
	}
	
	local_car.susp_angle_fwd = 7*local_car_acceleration;
	local_car._position[0] += local_car.velocity*sin(local_car.direction-M_PI/2);
	local_car._position[2] += local_car.velocity*cos(local_car.direction-M_PI/2);
	

	if (keys['W']) {
		c_vel_fwd += fwd_modifier;
	}
	if (keys['S']) {
		c_vel_fwd -= fwd_modifier;
	}	
	
	c_vel_fwd *= 0.97;

	if (keys['A']) {
		c_vel_side -= side_modifier;
	}
	if (keys['D']) {
		c_vel_side += side_modifier;
	}
	c_vel_side *= 0.95;
		if (keys['N']) {
			keys['N'] = false;
		}

		if (keys['P']) {
			PMODE = (PMODE == GL_FILL ? GL_LINE : GL_FILL);
			keys['P'] = false;
		}

		if (keys['T']) {
			keys['T'] = false;
		}
		
	

		if (dy != 0) {
			rotatey(mouse_modifier*dy);
		}
		if (dx != 0) {
			rotatex(mouse_modifier*dx);
		}


	}
	if (keys[VK_PRIOR]) {
		onScreenLog::scroll(12.0);
		keys[VK_PRIOR] = false;
	}
	if (keys[VK_NEXT]) {
		onScreenLog::scroll(-12.0);	
		keys[VK_NEXT] = false;
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

void initializeStrings() {

	// NOTE: it wouldn't be such a bad idea to just take in a vector 
	// of strings, and to generate one single static VBO for them all.

	wpstring_holder::append(wpstring("car. windows.", 15, 15), WPS_STATIC);
	wpstring_holder::append(wpstring("Frames per second: ", WINDOW_WIDTH-180, WINDOW_HEIGHT-20), WPS_STATIC);

	// reserved index 2 for FPS display. 
	const std::string initialfps = "00.00";
	wpstring_holder::append(wpstring(initialfps, WINDOW_WIDTH-50, WINDOW_HEIGHT-20), WPS_DYNAMIC);
	wpstring_holder::append(wpstring("", 12, HALF_WINDOW_HEIGHT), WPS_DYNAMIC);	

	wpstring_holder::createBufferObjects();

}

void drawText() {
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);

	glBindBuffer(GL_ARRAY_BUFFER, wpstring_holder::get_static_VBOid());

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(0));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(2*sizeof(float)));
	
	glUseProgram(text_shader->getProgramHandle());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, text_texId);

	text_shader->update_uniform_1i("texture1", 0);
	text_shader->update_uniform_mat4("ModelView", (const GLfloat*)text_ModelView.rawData());
	text_shader->update_uniform_mat4("Projection", (const GLfloat*)text_Projection.rawData());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wpstring_holder::get_IBOid());
	
	glDrawElements(GL_TRIANGLES, 6*wpstring_holder::get_static_strings_total_length(), GL_UNSIGNED_SHORT, NULL);
		
	glBindBuffer(GL_ARRAY_BUFFER, wpstring_holder::get_dynamic_VBOid());
	// not sure if needed or not
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(0));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(2*sizeof(float)));

	glUseProgram(text_shader->getProgramHandle());
	text_shader->update_uniform_1i("texture1", 0);
	text_shader->update_uniform_mat4("ModelView", (const GLfloat*)text_ModelView.rawData());
	text_shader->update_uniform_mat4("Projection", (const GLfloat*)text_Projection.rawData());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wpstring_holder::get_IBOid());
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, text_texId);

	glDrawElements(GL_TRIANGLES, 6*wpstring_holder::getDynamicStringCount()*WPSTRING_LENGTH_MAX, GL_UNSIGNED_SHORT, NULL);

	glUseProgram(0);
	glEnable(GL_DEPTH_TEST);

}

void print_to_GL_window(const std::string &text) {
	wpstring_holder::updateDynamicString(1, text, wpstring_holder::getDynamicString(1).getActualSize());	
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
	initializeStrings();
	
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
	earth_tex_id = TextureBank::get_id_by_name("textures/EarthTexture.jpg");
	hmap_id = TextureBank::get_id_by_name("textures/earth_height_normal_map.jpg");
	text_texId = TextureBank::get_id_by_name("textures/dina_all.png");
	
	regular_shader = new ShaderProgram("shaders/regular"); 
	normal_plot_shader = new ShaderProgram("shaders/normalplot");
	text_shader = new ShaderProgram("shaders/text_shader");

	if (regular_shader->is_bad() || normal_plot_shader->is_bad() || text_shader->is_bad()) { 
		onScreenLog::print( "Error: shader error (fatal).\n");
		return 0; 
	}


	glGenBuffers(1, &IBOid);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOid);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short int)*3*facecount, indices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*(GLushort)(0xFFFF), indices, GL_STATIC_DRAW);

	delete [] indices;	

	glPatchParameteri(GL_PATCH_VERTICES, 3);

	//GLuint programHandle = regular_shader->getProgramHandle();

	//glBindFragDataLocation(programHandle, 0, "out_frag_color");
	regular_shader->construct_uniform_map();
	normal_plot_shader->construct_uniform_map();
	text_shader->construct_uniform_map();


	//GLuint fragloc = glGetFragDataLocation( programHandle, "out_frag_color"); uniform_assert_warn(fragloc);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);


	view = mat4::identity();

	projection = mat4::proj_persp(M_PI/8.0, (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 2.0, 1000.0);

	view_position = vec4(0.0, 0.0, -9.0, 1.0);
	cameraVel = vec4(0.0, 0.0, 0.0, 1.0);
		
	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	//wglSwapIntervalEXT(2); // prefer hardware-forced vsync over this

	models.push_back(Model(1.0, 1.0, chassis.VBOid, earth_tex_id, chassis.facecount, true, false));
	models.push_back(Model(1.0, 1.0, wheel.VBOid, earth_tex_id, wheel.facecount, true, false));

	glBindBuffer(GL_ARRAY_BUFFER, chassis.VBOid);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(0));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(3*sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(6*sizeof(float)));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOid);

	// fbo generation for shadow mapping stuff.

	glGenFramebuffers(1, &FBOid);
	glBindFramebuffer(GL_FRAMEBUFFER, FBOid);

	glGenTextures(1, &FBO_textureId);
	glBindTexture(GL_TEXTURE_2D, FBO_textureId);

static int SHADOW_MAP_WIDTH = 1*WINDOW_WIDTH;
static int SHADOW_MAP_HEIGHT = 1*WINDOW_HEIGHT;

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );	// these two are related to artifact mitigation
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, FBO_textureId, 0);

	glDrawBuffer(GL_NONE);	// this, too, has something to do with only including the depth component 
	glReadBuffer(GL_NONE);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		onScreenLog::print( "error: Shadow-map FBO initialization failed!\n");
		return 0;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	return 1;

}

void drawPlane() {
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUseProgram(regular_shader->getProgramHandle());

	static const mat4 modelview = mat4::identity();//mat4::scale(100.0, 100.0, 100.0);
	regular_shader->update_uniform_mat4("ModelView", modelview.rawData());
	
	regular_shader->update_uniform_mat4("Projection", (const GLfloat*)projection.rawData());
	glBindBuffer(GL_ARRAY_BUFFER, plane.VBOid);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(0));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(3*sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(6*sizeof(float)));

	glDrawElements(GL_TRIANGLES, plane.facecount*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
}


void drawCar(const Car &car) {
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, PMODE);
	glUseProgram(regular_shader->getProgramHandle());	

	running += 0.015;
	if (running > 1000000) { running = 0; }

	regular_shader->update_uniform_1f("running", running);
	regular_shader->update_uniform_mat4("Projection", (const GLfloat*)projection.rawData());

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOid);  // is still in full matafaking effizzect :D 
	glBindBuffer(GL_ARRAY_BUFFER, chassis.VBOid);	 

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(0));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(3*sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(6*sizeof(float)));

	viewq.normalize();
	view = viewq.toRotationMatrix();
	view = view*mat4::translate(view_position);

	static float direction = 0.0;	// in the xz-plane

	// no need to reconstruct iterator every time
	static std::vector<Model>::iterator current;
	current = models.begin();

	static const vec4 chassis_color(0.8, 0.3, 0.5, 1.0);
	static const vec4 wheel_color(0.07, 0.07, 0.07, 1.0);

	mat4 modelview = view * mat4::translate(car.position()) * mat4::rotate(-car.direction, 0.0, 1.0, 0.0);
	mat4 mw = modelview*mat4::rotate(car.susp_angle_roll, 1.0, 0.0, 0.0);
	mw *= mat4::rotate(car.susp_angle_fwd, 0.0, 0.0, 1.0);
	vec4 light_dir = view * vec4(0.0, 1.0, 1.0, 0.0);

	regular_shader->update_uniform_vec4("light_direction", light_dir.rawData());
	regular_shader->update_uniform_mat4("ModelView", mw.rawData());
	regular_shader->update_uniform_vec4("paint_color", chassis_color.rawData());
	glBindBuffer(GL_ARRAY_BUFFER, chassis.VBOid);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, (*current).getTextureId());
	regular_shader->update_uniform_1i("texture_color", 0);

	//modelview.print();

	glDisable(GL_BLEND);
	glDrawElements(GL_TRIANGLES, chassis.facecount*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 

	glBindBuffer(GL_ARRAY_BUFFER, wheel.VBOid);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(0));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(3*sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(6*sizeof(float)));
	
	regular_shader->update_uniform_vec4("paint_color", wheel_color.rawData());
	
	static float wheel_angle = 0.0;
	wheel_angle -= 1.05*car.velocity;

	// front wheels
	static const mat4 front_left_wheel_translation = mat4::translate(vec4(-2.2, -0.6, 0.9, 1.0));
	mw = modelview;
	mw *= front_left_wheel_translation;
	mw *= mat4::rotate(M_PI - car.front_wheel_angle, 0.0, 1.0, 0.0);
	mw *= mat4::rotate(-wheel_angle, 0.0, 0.0, 1.0);


	regular_shader->update_uniform_mat4("ModelView", mw.rawData());
	glDrawElements(GL_TRIANGLES, wheel.facecount*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 
	
	static const mat4 front_right_wheel_translation = mat4::translate(vec4(-2.2, -0.6, -0.9, 1.0));
	mw = modelview;
	mw *= front_right_wheel_translation;
	mw *= mat4::rotate(-car.front_wheel_angle, 0.0, 1.0, 0.0);
	mw *= mat4::rotate(wheel_angle, 0.0, 0.0, 1.0);


	regular_shader->update_uniform_mat4("ModelView", mw.rawData());
	glDrawElements(GL_TRIANGLES, wheel.facecount*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 

	// back wheels
	static const mat4 back_left_wheel_translation = mat4::translate(vec4(1.3, -0.6, 0.9, 1.0));
	mw = modelview;
	mw *= back_left_wheel_translation;
	mw *= mat4::rotate(wheel_angle, 0.0, 0.0, 1.0);
	mw *= mat4::rotate(M_PI, 0.0, 1.0, 0.0);

	
	regular_shader->update_uniform_mat4("ModelView", mw.rawData());
	glDrawElements(GL_TRIANGLES, wheel.facecount*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 
	
	static const mat4 back_right_wheel_translation = mat4::translate(vec4(1.3, -0.6, -0.9, 1.0));

	mw = modelview;
	mw *= back_right_wheel_translation;
	mw *= mat4::rotate(wheel_angle, 0.0, 0.0, 1.0);

	regular_shader->update_uniform_mat4("ModelView", mw.rawData());
	glDrawElements(GL_TRIANGLES, wheel.facecount*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 
}


static std::string get_fps(long us_per_frame) {
	double f = 1000000/us_per_frame;
	static char fps[8];
	sprintf_s(fps, 8, "%4.2f", f);
	return std::string(fps);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	if(!CreateGLWindow("opengl framework stolen from NeHe", WINDOW_WIDTH, WINDOW_HEIGHT, 32, FALSE)) { return 1; }
	
	std::string cpustr(checkCPUCapabilities());
	if (cpustr.find("ERROR") != std::string::npos) { MessageBox(NULL, cpustr.c_str(), "Fatal error.", MB_OK); return -1; }

	MSG msg;
	BOOL done=FALSE;

	LocalClient::init("Jarmo2", "127.0.0.1", (unsigned short)50000);

	wglSwapIntervalEXT(1);

	onScreenLog::print("%s\n", cpustr.c_str());
	//ShowCursor(FALSE);

	bool esc = false;
	
	onScreenLog::print("moro, ma oon melkein vittupaa!!!!\nkyrpa!!\n");

	_timer timer;
	
	while(!done)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
			{
				fprintf(stderr, "Sending quit message (C_QUIT) to server.\n");
				LocalClient::quit();
				
				done=TRUE;
			}
			else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else {
			if (active)
			{
				if(keys[VK_ESCAPE])
				{
					if (!esc) {
						mouseLocked = !mouseLocked;
						ShowCursor(mouseLocked ? FALSE : TRUE);
						esc = true;
					}
					//done=TRUE;
				}
				else{
					esc=false;

					control();
					update_c_pos();
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					drawCar(local_car);
					drawText();
					
					onScreenLog::draw();

					window_swapbuffers();
					long us_per_frame = timer.get_us();
					std::string fps_str = get_fps(us_per_frame);

					wpstring_holder::updateDynamicString(0, fps_str);

					timer.begin();
				}
			}
		}

	}

	KillGLWindow();
	glDeleteBuffers(1, &IBOid);
	return (msg.wParam);
}


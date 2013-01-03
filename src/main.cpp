#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <unistd.h>
#include <getopt.h>
#include <signal.h>

#include "common.h"

#include "net/server.h"
#include "net/client.h"

#define WIN_W 800
#define WIN_H 600

float rotx = 0.0;
float rotz = 0.0;

std::stringstream sstream;

static unsigned int port = 31111;

static _timer timer;

int do_server_flag = 0, do_client_flag = 0;

static struct client local_client;

static void signal_handler(int signum);
static int clean_up_and_quit();

int string_to_int(const std::string &string) {
	sstream.str("");
	sstream.clear();
	int r;
	sstream << string;
	sstream >> r;
	return r;
}

std::string int_to_string(int i) {
	sstream.str("");
	sstream.clear();

	std::string r;
	sstream << i;
	sstream >> r;
	return r;
}

float sqrt_thirty_recip = 0.18257418583505537115;
// f(x) = -(1/(x+1/sqrt(30))^2) + 30
// f(x) = 1/(-x + 1/sqrt(30))^2 - 30

float f_wheel_angle(float x) {
	float t;
	if (x >= 0) {
		t = x+sqrt_thirty_recip;
		t *= t;
		return -(1/t) + 30;
	}
	else {
		t = -x+sqrt_thirty_recip;
		t *= t;
		return (1/t) - 30;
	}
}

GLuint car_list;
GLuint rengas_list;
GLuint skybox_list;
GLuint texture_car;
GLuint texture_tire;
GLuint texture_skybox;


mat4 rotateY(float angle) {	// in radian
	float tmp[16] = { cos(angle), 0, -sin(angle), 0,
		0, 1, 0, 0,
		sin(angle), 0, cos(angle), 0,
		0, 0, 0, 1 };
	mat4 M(tmp);
	return M;
}

static const float dt = 0.1;

std::streampos fileSize( const char* filePath ){

	std::streampos fsize = 0;
	std::ifstream file( filePath, std::ios::binary );

	fsize = file.tellg();
	file.seekg( 0, std::ios::end );
	fsize = file.tellg() - fsize;
	file.close();

	return fsize;
}

int initGL() {
	//GLenum err = glewInit();
	//if (GLEW_OK != err) {
	//	std::cerr << "glew init failed. lolz.\n";
	//}

	glEnable( GL_TEXTURE_2D );
	glEnable( GL_DEPTH_TEST);

	GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat mat_shininess[] = { 50.0 };
	GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
	GLfloat skybox_ambient[] = {1.0, 1.0, 1.0, 1.0 };
	glClearColor (0.0, 0.0, 0.0, 0.0);
	//glShadeModel (GL_SMOOTH);
	glShadeModel(GL_FLAT);

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT1, GL_AMBIENT, skybox_ambient);

	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glViewport(0, 0, WIN_W, WIN_H);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective(70, (float)WIN_H/(float)WIN_W, 3, 500);
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	return 1;
}

vertex *loadBobj(const char* filename, size_t *num) {

	std::ifstream infile(filename, std::ios::binary);
	if (!infile.is_open()) { std::cerr << "FATAL: Couldn't open file \"" << filename << "\"\n"; infile.close(); exit(1); }

	std::size_t filesize = fileSize(filename);
	infile.seekg(4, std::ios::beg);
	int num_vertices;
	infile.read((char*)&num_vertices, sizeof(int));
	std::cout << "file name: " << filename << " - num_vertices " << num_vertices << ", filesize: " <<  filesize << "\n";
	vertex *vertices = new vertex[num_vertices];
	infile.seekg(8, std::ios::beg);
	infile.read((char*)vertices, filesize - 8);
	infile.close();

	*num = num_vertices;

	return vertices;

}

GLuint genList(const char* filename) {
	size_t num_vertices;
	vertex *vertices = loadBobj(filename, &num_vertices);
	GLuint list_id = glGenLists(1);
	glNewList(list_id, GL_COMPILE);
	glBegin(GL_TRIANGLES);
	for (size_t i = 0; i < num_vertices; i++) {
		glVertex3f(vertices[i].vx, vertices[i].vy, vertices[i].vz);
		glNormal3f(vertices[i].nx, vertices[i].ny, vertices[i].nz);
		glTexCoord2f(vertices[i].u, vertices[i].v);
	}
	glEnd();
	glEndList();
	delete [] vertices;

	return list_id;
}

int genLists() {
	car_list = genList("models/auto.bobj");
	rengas_list = genList("models/rengas.bobj");
	skybox_list = genList("models/skybox.bobj");
	return 1;
}

char* loadTextureData(const char* filename, std::size_t *size) {

	unsigned long filesize = fileSize(filename);
	char *data = new char[filesize];
	std::ifstream infile(filename, std::ios::binary);
	infile.seekg(14, std::ios::beg);	
	infile.read(data, filesize-14);
	infile.close();
	*size = filesize;
	return data;

}


int genTexture() {

	glGenTextures(1, &texture_car);
	glBindTexture(GL_TEXTURE_2D, texture_car);
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	//	unsigned long filesize = fileSize("kangas.bmp");
	std::size_t filesize;
	char* data = loadTextureData("textures/cartexture.bmp", &filesize);
	gluBuild2DMipmaps( GL_TEXTURE_2D, 3, 512, 512, GL_BGR, GL_UNSIGNED_BYTE, data);
	delete [] data;

	glGenTextures(1, &texture_tire);
	glBindTexture(GL_TEXTURE_2D, texture_tire);
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	//	unsigned long filesize = fileSize("kangas.bmp");
	const long g = 3*128*128;
	data = new char[g];
	memset(data, 51, g);
	//	gluBuild2DMipmaps( GL_TEXTURE_2D, 3, 1024, 1024, GL_BGR, GL_UNSIGNED_BYTE, data);
	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, 32, 32, GL_BGR, GL_UNSIGNED_BYTE, data);
	delete [] data;

	data = loadTextureData("textures/skybox.bmp", &filesize);
	glGenTextures(1, &texture_skybox);
	glBindTexture(GL_TEXTURE_2D, texture_skybox);
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, 2048, 2048, GL_BGR, GL_UNSIGNED_BYTE, data);
	delete [] data;

}

void drawcars(float* camM) {

	id_client_map& peers = client_get_peers();
	id_client_map::iterator iter = peers.begin();
	while (iter != peers.end()) {
		struct car &c = (*iter).second.lcar;
//		std::cerr << "drawing " << iter->name << "'s car at position " << c.position.x << " " << c.position.z << "\n";
		mat4 O = rotateY(c.direction);
		/* chassis */
		glEnable(GL_LIGHT0);
		glDisable(GL_LIGHT1);

		glBindTexture(GL_TEXTURE_2D, texture_car);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixf(camM);
		glTranslatef(c.position.x, 0.0, c.position.z);

		glMultMatrixf(O.data);
		glRotatef(c.susp_angle_roll, 1.0, 0.0, 0.0);
		glRotatef(c.susp_angle_fwd, 0.0, 0.0, 1.0);
		glCallList(car_list);

		static float wheel_angle = 0;
		wheel_angle -= c.velocity * 71.0;

		glBindTexture(GL_TEXTURE_2D, texture_tire);
		/* front wheels */
		glMatrixMode(GL_MODELVIEW);

		glLoadIdentity();
		glMultMatrixf(camM);
		glTranslatef(c.position.x, 0.0, c.position.z);
		glMultMatrixf(O.data);
		glTranslatef(-2.2, -0.6, 0.9);
		glRotatef(180+c.fw_angle, 0.0, 1.0, 0.0);
		glRotatef(wheel_angle, 0.0, 0.0, 1.0);
		glCallList(rengas_list);

		glLoadIdentity();
		glMultMatrixf(camM);
		glTranslatef(c.position.x, 0.0, c.position.z);
		glMultMatrixf(O.data);
		glTranslatef(-2.2, -0.6, -0.9);
		glRotatef(c.fw_angle, 0.0, 1.0, 0.0);
		glRotatef(-wheel_angle, 0.0, 0.0, 1.0);
		glCallList(rengas_list);

		/* back wheels */
		glLoadIdentity();
		glMultMatrixf(camM);
		glTranslatef(c.position.x, 0.0, c.position.z);
		glMultMatrixf(O.data);
		glTranslatef(1.3, -0.6, 0.9);
		glRotatef(180, 0.0, 1.0, 0.0);
		glRotatef(wheel_angle, 0.0, 0.0, 1.0);
		glCallList(rengas_list);

		glLoadIdentity();
		glMultMatrixf(camM);
		glTranslatef(c.position.x, 0.0, c.position.z);
		glMultMatrixf(O.data);
		glTranslatef(1.3, -0.6, -0.9);
		glRotatef(-wheel_angle, 0.0, 0.0, 1.0);
		glCallList(rengas_list);

		//glLoadIdentity();
		//glMultMatrixf(camM);
		//glTranslatef(c.position.x, 3.0, c.position.z);
		//face->draw(0, 0, (*iter).second.name.c_str());

		++iter;
	}

}

void process_input() {

	Uint8* keystate = SDL_GetKeyState(NULL);

	struct car& lcar = local_client.lcar;

	static float local_car_acceleration = 0.0;
	static float local_car_prev_velocity;
	local_car_prev_velocity = lcar.velocity;

	if (keystate[SDLK_UP]) {
		lcar.velocity += 0.015;
	}
	else {
		lcar.velocity *= 0.95;
		lcar.susp_angle_roll *= 0.90;
		lcar.tmpx *= 0.50;
	}

	if (keystate[SDLK_DOWN]) {
		if (lcar.velocity > 0.01) {
			// simulate handbraking slides :D
			lcar.direction -= 3*lcar.F_centripetal*lcar.velocity*0.20;
			lcar.velocity *= 0.99;				
		}
		lcar.velocity -= 0.010;
	}
	else {
		lcar.velocity *= 0.96;
		lcar.susp_angle_roll *= 0.90;
		lcar.tmpx *= 0.80;
	}

	if (keystate[SDLK_LEFT]) {
		if (lcar.tmpx < 0.15) {
			lcar.tmpx += 0.05;
		}
		lcar.F_centripetal = -0.5;
		lcar.fw_angle = f_wheel_angle(lcar.tmpx);
		lcar.susp_angle_roll = -lcar.fw_angle*fabs(lcar.velocity)*0.50;
		if (lcar.velocity < 0) {
			lcar.direction += 0.046*lcar.fw_angle*lcar.velocity*0.20;
		}
		else {
			lcar.direction += 0.031*lcar.fw_angle*lcar.velocity*0.20;
		}
	
	}
	if (keystate[SDLK_RIGHT]) {
		if (lcar.tmpx > -0.15) {
			lcar.tmpx -= 0.05;
		}
		lcar.F_centripetal = 0.5;
		lcar.fw_angle = f_wheel_angle(lcar.tmpx);
		lcar.susp_angle_roll = -lcar.fw_angle*fabs(lcar.velocity)*0.50;

		if (lcar.velocity < 0) {
			lcar.direction += 0.046*lcar.fw_angle*lcar.velocity*0.20;
		}
		else {
			lcar.direction += 0.031*lcar.fw_angle*lcar.velocity*0.20;
		}

	}
	local_car_prev_velocity = 0.5*(lcar.velocity+local_car_prev_velocity);
	local_car_acceleration = 0.2*(lcar.velocity - local_car_prev_velocity) + 0.8*local_car_acceleration;

	if (!keystate[SDLK_LEFT] && !keystate[SDLK_RIGHT]){
		lcar.tmpx *= 0.30;
		lcar.fw_angle = f_wheel_angle(lcar.tmpx);
		lcar.susp_angle_roll *= 0.50;
		lcar.F_centripetal = 0;
		lcar.susp_angle_fwd *= 0.50;
	}
	lcar.susp_angle_fwd = -80*local_car_acceleration;
	lcar.position.x += lcar.velocity*sin(lcar.direction-M_PI/2);
	lcar.position.z += lcar.velocity*cos(lcar.direction-M_PI/2);

	if (keystate[SDLK_ESCAPE]) {
		signal_handler(SIGINT);
	}

}

void draw() {

	//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(rotz, 1.0, 0.0, 0.0);
	glRotatef(rotx, 0.0, 1.0, 0.0);
	glTranslatef(0.0, -3.0, -8.0);
	static GLfloat camM[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, camM);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(40, (float)WIN_W/(float)WIN_H, 3, 500);

	drawcars(camM);

	glDisable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glBindTexture(GL_TEXTURE_2D, texture_skybox);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf(camM);
	glScalef(100.0, 100.0, 100.0);
	glCallList(skybox_list);

	/*int fps = 1000.0/(float)dt;
	std::ostringstream stream;
	stream << fps;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, WIN_W, 0, WIN_H, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	face->draw(15, 15, (stream.str() + " FPS").c_str()); */
}

static void process_events( void )
{
	/* Our SDL event placeholder. */
	SDL_Event event;

	static float prev_mpos[2];
	static float cur_mpos[2];
	/* Grab all the events off the queue. */
	while( SDL_PollEvent( &event ) ) {

		switch( event.type ) {
			case SDL_MOUSEMOTION:
				if (event.motion.x == WIN_W/2 && event.motion.y == WIN_H/2) { break; }
				cur_mpos[0] = event.motion.x;
				cur_mpos[1] = event.motion.y;

				rotx -= 0.01*(float)(WIN_W/2 - cur_mpos[0]);
				rotz += 0.01*(float)(WIN_H/2 - cur_mpos[1]);

				//				std::cout << rotx << " " << rotz << "\n";
				SDL_WarpMouse(WIN_W/2, WIN_H/2);
				break;

			case SDL_QUIT:
				/* Handle quit requests (like Ctrl-c). */
				exit(0);
				break;
		}

	}

}

/* the addr string is of format "a.b.c.d:<port>" */
struct ip_port_struct ip_port_struct_from_addr(const char* addr) {

	struct ip_port_struct r;
	std::string copy = std::string(addr);

	std::vector<std::string> tokens = tokenize(copy, ':');
	std::string ipstr = tokens[0];
	r.address = strdup(ipstr.c_str()); 
	std::cerr << "r.address: " << r.address << "\n";

	r.port = string_to_int(tokens[1]);
	std::cout << "port: " << r.port << "\n";

	return r;

}

static struct option long_options[] =
{
	{ "server", required_argument, &do_server_flag, 's' },
	{ "connect", required_argument, &do_client_flag, 'c' },
	{ NULL, 0, NULL, 0 }
};


struct ip_port_struct connect_flag_action(const char* arg) {
	struct ip_port_struct addr = ip_port_struct_from_addr(arg);
	do_client_flag = 1;
	return addr;
}

void server_flag_action(const char* arg) {
	std::stringstream stream;
	stream.clear();
	stream.str(optarg);
	stream >> port;
	do_server_flag = 1;

}

std::vector<std::string> tokenize(const std::string& str, char delim, unsigned tmax) {
	// c++ std::string::find-based implementation
	std::vector<std::string> tokens;
	size_t found_index = 0;
	size_t prev_index = 0;

	unsigned i = 0;
	found_index = str.find(delim, found_index);

	while (found_index != std::string::npos && i < tmax) {
		std::string token = str.substr(prev_index, found_index - prev_index);
//		std::cerr << token << " at " << found_index << "\n";
		tokens.push_back(token);
		++i;
		prev_index = found_index + 1;
		found_index = str.find(delim, found_index+1);
	}
	if (str.length() > prev_index + 1) {
		std::string last_token = str.substr(prev_index, str.length() - prev_index);
		tokens.push_back(last_token);
//		std::cerr << last_token << " at " << prev_index << "\n";
//		std::cerr << "tokenize: found " << tokens.size() << " tokens total.\n";
	}

	return tokens;

}

void send_key_state() {
	/* FORMAT:
	 * CLINPST:<CL_ID>:<INPUT_STATE_BITMASK>
	 */
	// INPUT_STATE_BITMASK:
	// UP: 		00000001
	// DOWN:	00000010
	// LEFT:	00000100
	// RIGHT:	00001000
	
	unsigned char st = 0;
	Uint8* keystate = SDL_GetKeyState(NULL);

	if (keystate[SDLK_UP]) {
		st |= KEY_UP;
	}
	if (keystate[SDLK_DOWN]) {
		st |= KEY_DOWN;
	}
	if (keystate[SDLK_LEFT]) {
		st |= KEY_LEFT;
	}
	if (keystate[SDLK_RIGHT]) {
		st |= KEY_RIGHT;
	}

	if (keystate[SDLK_ESCAPE]) {
		signal_handler(SIGINT);
	}

	client_send_input_state_to_server(st);
}

static void signal_handler(int signum) {
	if (do_client_flag) {
		client_post_quit_message();
		exit(clean_up_and_quit());
	}
	else if (do_server_flag) { 
		server_post_quit_message();
		exit(clean_up_and_quit());
	}
	else {
		exit(clean_up_and_quit());
	}
}

int init_SDL() {

	const SDL_VideoInfo* info = NULL;
	int bpp = 0;
	int flags = 0;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "Video initialization failed: " << SDL_GetError() << "\n";
		return -1;
	}

	info = SDL_GetVideoInfo();

	if(!info) {
		std::cerr << "Video query failed: " << SDL_GetError() << "\n";
		return -1;
	}

	bpp = info->vfmt->BitsPerPixel;

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	flags = SDL_OPENGL | SDL_HWSURFACE;

	if(SDL_SetVideoMode(WIN_W, WIN_H, bpp, flags) == 0) {
		fprintf(stderr, "Video mode set failed: %s\n", SDL_GetError());
		return -1;
	}

	int k = initGL();
	if (initGL() < 0) { return -1; }
	genTexture();
	genLists();
}

static int clean_up_and_quit() {
	SDL_Quit();
	return 0;
}

int main(int argc, char* argv[]) {

	int option_index;
	struct ip_port_struct addr;
	struct ip_port_struct remote;
	extern char *optarg;	// don't know what these are :D
	extern int optind, opterr, optopt;

	signal(SIGINT, signal_handler);

	int c;
	while ((c = getopt_long(argc, argv, "s:c:", long_options, &option_index)) != -1) {
		switch(c) {
			case 0:
				std::cout << long_options[option_index].name << " " << optarg << " supplied\n";
				if (strcmp(long_options[option_index].name, "connect") == 0) {
					remote = connect_flag_action(optarg);
					do_client_flag = 1;
				}
				else if (strcmp(long_options[option_index].name, "server") == 0){
					server_flag_action(optarg);
					do_server_flag = 1;
				}
				break;

			case 's':
				std::cout << "-s supplied.\n";
				server_flag_action(optarg);
				do_server_flag = 1;
				break;

			case 'c':
				std::cout << "-c supplied.\n";
				remote = connect_flag_action(optarg);
				do_client_flag = 1;
				break;

			case '?':
				std::cout << "wtf.\n";
				break;
			case -1:
				break;

		}


	}
	std::cerr << "do_server_flag: " << do_server_flag << ", do_client_flag: " << do_client_flag << "\n";

	if (do_server_flag && do_client_flag ) {
		std::cerr << "Both client AND server can't be launched at the same time.\n";
		exit(1);
	}
	if (do_server_flag) {
		std::cerr << "starting server on port " << port <<"\n";
		server_start(port);
	}
	else if (do_client_flag) {
		if (client_start(remote.address, remote.port) < 0) {
			std::cerr << "client_start failed. Exiting.\n";
		}
	}


	// separate loops for client and server "clients" :D
	if (do_client_flag) {
		if (init_SDL() < 0) { exit(1); };
		while(true) {
			timer.begin();
			process_events();
			//process_input();
			send_key_state();
			if (client_get_data_from_remote() < 0) { signal_handler(SIGINT); }// is also processed in this call
			draw();
			SDL_GL_SwapBuffers();
			time_t us = timer.get_us();

			while (us < 16666) {
				us = timer.get_us();
			}
			//std::cerr << 1000000/us << "\n";


		}
	}
	else if (do_server_flag) {
		while(true) {
//			process_events();
//			process_input();
			
			server_receive_packets();	// packets are also processed in this call

		}
	}
	else {
		/* local "play" */
		if (init_SDL() < 0) { exit(1); };

		id_client_map &peers = client_get_peers();
		peers[1] = local_client;
		id_client_map::iterator it = peers.find(1);

		while(1) {
			timer.begin();
			it->second = local_client;
			process_events();
			process_input();

			time_t us = timer.get_us();

			draw();
			SDL_GL_SwapBuffers();
			while (us < 16666) {
				us = timer.get_us();
			}
		}
	}
	return 0;

}

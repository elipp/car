#include "server.h"

static unsigned char packet_data[256];
static unsigned char interm_buf[256];
static size_t maximum_packet_size = sizeof(packet_data);

static int sockfd, new_fd;
static struct sockaddr_in my_addr;

static id_client_map clients;
static _timer timer;

extern std::stringstream sstream;

int server_createUDPSocket(unsigned short int port) {
	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd <= 0) {
		std::cerr << "multip: Failed to create socket.\n";
		return -1;
	}
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons((unsigned short) port);

	if (bind(sockfd, (const sockaddr*)&my_addr, sizeof(struct sockaddr_in) ) < 0 ) {
		std::cerr < "multip: Failed to bind socket.\n";
		return -1;
	}
	if (fcntl(sockfd, F_SETFL, O_NONBLOCK, 1) == -1) {
		printf( "server: failed to set non-blocking socket\n" );
		return -1;
	}

	// consider non-blocking
	return 1;
}

struct sockaddr_in get_local_address() { return my_addr; }

int server_add_client(struct sockaddr_in *si_from) {

	static int num_clients = 0;
	struct client newclient;

	// we are expecting (and assuming) C_HANDSHAKE (protocol.h) as first byte, and port as the next two bytes
	
	newclient.port = 0;
	newclient.port |= packet_data[2];
	newclient.port |= ((unsigned short)(packet_data[1]) << 8) & 0xFF00;
	// handshake[3] represents the length of the following name string
	std::cerr << "server_add_client: extracted port " << newclient.port << " from handshake.\n";

	size_t name_len = packet_data[3];
	memcpy(interm_buf, &packet_data[4], name_len);
	interm_buf[name_len] = '\0';
	newclient.name = std::string((const char*)interm_buf);

	const char* ip_str = inet_ntoa(si_from->sin_addr);
	newclient.ip_string = std::string(ip_str);

	num_clients++;

	newclient.id = num_clients;
	newclient.address = *si_from;
	clients.insert(id_client_pair(newclient.id, newclient));
//	clients[newclient.id] = newclient;	
	
	return newclient.id;
}

struct client *get_client_by_id(int id) {
	id_client_map::iterator it = clients.find(id);
	if (it != clients.end()) {
		return &(*it).second;
	}
	else {
		return NULL;
	}
}

int server_process_packet(struct sockaddr_in *from) {

	unsigned int from_address = ntohl(from->sin_addr.s_addr);
	unsigned short from_port = ntohs(from->sin_port);
	char *inet_addr = inet_ntoa(from->sin_addr);

	if (packet_data[0] == C_HANDSHAKE) {
		std::cerr << "client @ " << inet_addr << ":" << from_port << " :handshake request\n";
		int newclient_id = server_add_client(from);
		if (newclient_id < 0) { std::cerr << "server_add_client: error.\n"; return -1; }
		interm_buf[0] = S_HANDSHAKE_OK;
		interm_buf[1] = newclient_id;
		interm_buf[2] = 0;
		struct client *newclient = get_client_by_id(newclient_id);
		server_send_packet(interm_buf, 2, newclient);
		server_post_peer_list();
	}
	else if (packet_data[0] == C_QUIT) {
		unsigned short id = packet_data[1];
		struct client *quitting_client = get_client_by_id(id);
		if (quitting_client) {
			std::cerr << "Client " << quitting_client->name << " with id " << quitting_client->id << " quitting.\n";
			server_remove_client(id);
		}
		else {
			std::cerr << "client with id " << id << "not found.\n";
		}
	}
	else if (packet_data[0] == C_KEYSTATE) {
		unsigned short id = packet_data[1];
		client *c = get_client_by_id(id);
		if (c) {
			c->keystate = packet_data[2];
			server_update_client_status(c);
			server_post_position_update(c);
		}
	}	
	else { 
		return 0;
	}
}

int server_receive_packets() {
	static size_t num_packets_received = 0;
	static size_t num_bytes_received = 0;
	static _timer timer2;
	time_t t = timer2.get_ns();
	if (t > 1000000000) {
		std::cerr << "packets received per second: " << num_packets_received << " (" << num_bytes_received << " B/s).\n";
		timer2.begin();
		num_packets_received = 0;
		num_bytes_received = 0;
	}
	
	struct sockaddr_in from;
	socklen_t from_length = sizeof(from);

	// the UDP socket is now int non-blocking mode, i.e. recvfrom returns 0 if no packets are to be read
	
//	memset(packet_data, 0, maximum_packet_size);	***
	memset(&from, 0, from_length);

	int received_bytes = recvfrom(sockfd, (char*)packet_data,
	maximum_packet_size, 0, (struct sockaddr*)&from, &from_length);
	packet_data[received_bytes] = '\0';

	num_bytes_received += received_bytes < 0 ? 0 : received_bytes;

	size_t num_packets = 0;

	if (received_bytes <= 0) { return 1; }
	// else:
	num_packets_received++;
	do {
		++num_packets;
		num_bytes_received += received_bytes < 0 ? 0 : received_bytes;
;
		server_process_packet(&from);

		memset(&from, 0, from_length);

		received_bytes = recvfrom(sockfd, (char*)packet_data,
		maximum_packet_size, 0, (struct sockaddr*)&from, &from_length);
		
		packet_data[received_bytes] = '\0';

	} while (received_bytes > 0);

	return 0;
}

int server_send_packet(unsigned char *data, size_t len, struct client *c) {
	static size_t num_packets_sent = 0;
	
	time_t t = timer.get_ns();
	if (t > 1000000000) {
		std::cerr << "packets sent per second: " << num_packets_sent << ".\n";
		timer.begin();
		num_packets_sent = 0;
	}
	memcpy(packet_data, data, len);
	packet_data[len] = '\0';
	int sent_bytes = sendto(sockfd, (const char*)packet_data, len,
	0, (struct sockaddr*)&c->address, sizeof(struct sockaddr));
	num_packets_sent++;

//	std::cerr << "sending \"" << (const char*)packet_data << "\"\n";
	
	return sent_bytes;

}


int server_post_quit_message() {

	interm_buf[0] = S_QUIT;
	interm_buf[1] = '\0';

	id_client_map::iterator iter = clients.begin();
	while (iter != clients.end()) {
		server_send_packet(interm_buf, 1, &(*iter).second);
		++iter;
	}
	return 1;
}

int server_update_client_status(client *c) {
	struct car &c_car = c->lcar;
	static float local_car_acceleration = 0.0;
	static float local_car_prev_velocity = c_car.velocity;
//	std::cerr << "server_update_positions: " << c->name << " direction: "  << c_car.direction << "\n";

	if (c->keystate & KEY_UP) {
		c_car.velocity += 0.015;
	}
	else {
		c_car.velocity *= 0.95;
		c_car.susp_angle_roll *= 0.90;
		c_car.tmpx *= 0.50;
	}

	if (c->keystate & KEY_DOWN) {
		if (c_car.velocity > 0.01) {
			// simulate handbraking slides :D
			c_car.direction -= 3*c_car.F_centripetal*c_car.velocity*0.20;
			c_car.velocity *= 0.99;				
		}
		c_car.velocity -= 0.010;
	}
	else {
		c_car.velocity *= 0.96;
		c_car.susp_angle_roll *= 0.90;
		c_car.tmpx *= 0.80;
	}

	if (c->keystate & KEY_LEFT) {
		if (c_car.tmpx < 0.15) {
			c_car.tmpx += 0.05;
		}
		c_car.F_centripetal = -0.5;
		c_car.fw_angle = f_wheel_angle(c_car.tmpx);
		c_car.susp_angle_roll = -c_car.fw_angle*fabs(c_car.velocity)*0.50;
		if (c_car.velocity < 0) {
			c_car.direction += 0.046*c_car.fw_angle*c_car.velocity*0.20;
		}
		else {
			c_car.direction += 0.031*c_car.fw_angle*c_car.velocity*0.20;
		}

	}
	if (c->keystate & KEY_RIGHT) {
		if (c_car.tmpx > -0.15) {
			c_car.tmpx -= 0.05;
		}
		c_car.F_centripetal = 0.5;
		c_car.fw_angle = f_wheel_angle(c_car.tmpx);
		c_car.susp_angle_roll = -c_car.fw_angle*fabs(c_car.velocity)*0.50;

		if (c_car.velocity < 0) {
			c_car.direction += 0.046*c_car.fw_angle*c_car.velocity*0.20;
		}
		else {
			c_car.direction += 0.031*c_car.fw_angle*c_car.velocity*0.20;
		}

	}
	local_car_prev_velocity = 0.5*(c_car.velocity+local_car_prev_velocity);
	local_car_acceleration = 0.2*(c_car.velocity - local_car_prev_velocity) + 0.8*local_car_acceleration;

	if (!(c->keystate & KEY_LEFT) && !(c->keystate & KEY_RIGHT)){
		c_car.tmpx *= 0.30;
		c_car.fw_angle = f_wheel_angle(c_car.tmpx);
		c_car.susp_angle_roll *= 0.50;
		c_car.F_centripetal = 0;
		c_car.susp_angle_fwd *= 0.50;
	}
	c_car.susp_angle_fwd = -80*local_car_acceleration;

	c_car.position.x += c_car.velocity*sin(c_car.direction-M_PI/2);
	c_car.position.z += c_car.velocity*cos(c_car.direction-M_PI/2);

	return 1;
}

int server_post_peer_list() {
	
	// <horrible workaround>
	std::string peer_list_str = "TT;";
	peer_list_str[0] = S_PEER_LIST;
	peer_list_str[1] = (char)clients.size();
	// </horrible workaround>

	id_client_map::iterator iter = clients.begin();
	while (true) {
		struct client &itclient = (*iter).second;
		peer_list_str = peer_list_str + int_to_string(itclient.id) + ":" + itclient.name + ":" + itclient.ip_string;
		++iter;
		if (iter != clients.end()) {
			peer_list_str = peer_list_str + ';';
		}
		else {
			break;
		}
	}
	iter = clients.begin();
	while (iter != clients.end()) {
		std::cerr << "server_post_peer_list: peer_list_str = \"" << peer_list_str << "\"\n";
		server_send_packet((unsigned char*)peer_list_str.c_str(), peer_list_str.length(), &(*iter).second);
		++iter;
	}
	return 1;
}

int server_remove_client(int id) {
	clients.erase(clients.find(id));
	return 1;
}

int server_start(unsigned short int port) {
	server_createUDPSocket(port);
	server_receive_packets();
	return 1;
}

int server_post_position_update(client *c) {
	//time_t ms = timer.get_ms();
	//std::cerr << "time since last pupd: " << ms << " ms.\n";
	//timer.begin();
	//memset(posupd_packet_buffer, 0, maximum_packet_size); ***
	interm_buf[0] = S_PUPD;
	interm_buf[1] = c->id;

	struct car_serialized cs = c->lcar.serialize();
	memcpy(&interm_buf[2], (const void*)&cs, sizeof(cs));

	size_t total_packet_size = 2 + sizeof(cs);
	interm_buf[total_packet_size] = '\0';


	id_client_map::iterator iter = clients.begin();
	while (iter != clients.end()) {
		server_send_packet(interm_buf, total_packet_size, &(*iter).second);
		++iter;
	}
}

#include "server.h"

static unsigned char packet_data[256];
static size_t maximum_packet_size = sizeof(packet_data);

static int sockfd, new_fd;
static struct sockaddr_in my_addr;

static id_client_map clients;

extern std::stringstream sstream;

int server_createUDPSocket(unsigned int port) {
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

int server_add_client(std::string handshake) {

	static int num_clients = 0;
	struct client newclient;
	std::vector<std::string> tokens = tokenize(handshake, ':');

	// format: "HANDSHAKE:<IPSTRING>:<NAME>:<PORT>"
	if (tokens.size() < 4) { std::cerr << "server_add_client: handshake tokenizing failed.\n"; return -1; }

	struct sockaddr_in si_other;
	memset(&si_other, 0, sizeof(si_other));

	newclient.port = string_to_int(tokens[3]);

	si_other.sin_family = AF_INET;
	si_other.sin_port = htons((unsigned short) newclient.port);

	newclient.ip_string = tokens[1];
	if (inet_aton(newclient.ip_string.c_str(), &si_other.sin_addr)==0) {
		std::cerr << "server_add_client: inet_aton failed. Invalid remote ip?\n";
		return NULL;
	}
	num_clients++;

	newclient.name = tokens[2];
	newclient.id = num_clients;
	newclient.id_string = int_to_string(newclient.id);
	newclient.address = si_other;
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

int server_receive_packets() {
	
	struct sockaddr_in from;
	socklen_t from_length = sizeof(from);

	// the UDP socket is now int non-blocking mode, i.e. recvfrom returns 0 if no packets are to be read
	
	memset(packet_data, 0, maximum_packet_size);
	
	int received_bytes = recvfrom(sockfd, (char*)packet_data,
	maximum_packet_size, 0, (struct sockaddr*)&from, &from_length);
	packet_data[maximum_packet_size-1] = '\0';

	if (received_bytes <= 0) { return 1; }
	// else:
	do {
		unsigned int from_address = ntohl(from.sin_addr.s_addr);
		unsigned int from_port = ntohs(from.sin_port);
		char *inet_addr = inet_ntoa(from.sin_addr);

		std::string packet_str = std::string((const char*)packet_data);

		if (packet_str.find("HANDSHAKE:") != std::string::npos) {
			std::cerr << "Client attempting to handshake. Message: \"" << packet_str << "\"\n";		
			int newclient_id = server_add_client(packet_str);
			std::string accept = "HANDSHAKE:OK:" + int_to_string(newclient_id);
			struct client *newclient = get_client_by_id(newclient_id);
			server_send_packet((unsigned char*)accept.c_str(), accept.length(), *newclient);
			server_post_peer_list();
		}
		else if (packet_str.find("QUIT:") != std::string::npos) {
			std::string quit_client_id = packet_str.substr(5, 1);
			int id = string_to_int(quit_client_id);
			struct client *quitting_client = get_client_by_id(id);
			if (quitting_client) {
				std::cerr << "Client " << quitting_client->name << " with id " << quitting_client->id << " quitting.\n";
				server_remove_client(id);
			}
			else {
				std::cerr << "client with id " << quit_client_id << "not found.\n";
			}
		}
		else if (packet_str.find("CLINPST:") != std::string::npos){
			std::vector<std::string> tokens = tokenize(packet_str, ':', 2);	// set tmax to 2, only extract id
			int id = string_to_int(tokens[1]);
			size_t st_pos = packet_str.find(';');
			client *c = get_client_by_id(id);
			if (c) {
				c->keystate = st_pos != std::string::npos ? packet_str[st_pos+1] : 0;
				server_update_client_status(c);
				server_post_position_update(c);
			}
		}	

		memset(packet_data, 0, maximum_packet_size);
		memset(&from, 0, from_length);

		received_bytes = recvfrom(sockfd, (char*)packet_data,
		maximum_packet_size, 0, (struct sockaddr*)&from, &from_length);
		

	} while (received_bytes > 0);

	return 0;
}

int server_send_packet(unsigned char *data, size_t len, struct client &c) {
	memset(packet_data, 0, maximum_packet_size);
	memcpy(packet_data, data, len);
	packet_data[len] = '\0';
	int sent_bytes = sendto(sockfd, (const char*)packet_data, len,
	0, (struct sockaddr*)&c.address, sizeof(struct sockaddr));
	
	return sent_bytes;

}


int server_post_quit_message() {
	
	id_client_map::iterator iter = clients.begin();
	std::string quit_msg = "SERVER:QUIT";
	while (iter != clients.end()) {
		server_send_packet((unsigned char*)quit_msg.c_str(), quit_msg.length(), (*iter).second);
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
			c_car.direction += 0.051*c_car.fw_angle*c_car.velocity*0.20;
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
	
	std::string peer_list_str = "SERVER:PEERS:" + int_to_string(clients.size()) + ';';

	id_client_map::iterator iter = clients.begin();
	while (true) {
		struct client &itclient = (*iter).second;
		peer_list_str = peer_list_str + itclient.id_string + ":" + itclient.name + ":" + itclient.ip_string;
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
		server_send_packet((unsigned char*)peer_list_str.c_str(), peer_list_str.length(), (*iter).second);
		++iter;
	}
	return 1;
}

int server_remove_client(int id) {
	clients.erase(clients.find(id));
	return 1;
}

int server_start(unsigned int port) {
	server_createUDPSocket(port);
	server_receive_packets();
	return 1;
}

int server_post_position_update(client *c) {
	unsigned char posupd_packet_buffer[maximum_packet_size];
	memset(posupd_packet_buffer, 0, maximum_packet_size);
	std::string posupd_str = "SERVER:PUPD:" + c->id_string + ';';
	size_t sep_index = posupd_str.find(';') + 1;

	memcpy(posupd_packet_buffer, posupd_str.c_str(), posupd_str.length());

	struct car_serialized cs = c->lcar.serialize();
	memcpy(posupd_packet_buffer+sep_index, (const void*)&cs, sizeof(cs));

	size_t total_packet_size = sep_index + sizeof(cs);
	posupd_packet_buffer[total_packet_size] = '\0';


	id_client_map::iterator iter = clients.begin();
	while (iter != clients.end()) {
		server_send_packet((unsigned char*)posupd_packet_buffer, total_packet_size, (*iter).second);
		++iter;
	}
}

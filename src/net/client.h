#ifndef CLIENT_H
#define CLIENT_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <cmath>
#include <vector>

#include <unistd.h>
#include <fcntl.h>

#include "../common.h"


struct client {
	struct sockaddr_in address;
	std::string ip_string;
	std::string name;
	int id;
	std::string id_string;
	unsigned int port;
	struct car lcar;
	unsigned char keystate;
};

int client_send_packet(unsigned char* input_data, size_t packet_size);
int client_start(std::string remote_ipstring, unsigned int remote_port);
int client_post_quit_message();
int client_send_input_state_to_server(unsigned char st);
int client_get_position_data();

int client_get_data_from_remote();
int client_process_data_from_remote();
int client_update_position_data();

struct client client_get_local_client();
int client_update_position_data(int player_id, unsigned char* data); 

id_client_map& client_get_peers();

#endif

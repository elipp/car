#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <cmath>
#include <unistd.h>
#include <fcntl.h>
#include <SDL/SDL.h>

#include "../common.h"
#include "client.h"
#include "protocol.h"

#define ADDR(a, b, c, d) ( a << 24 ) | ( b << 16 ) | ( c << 8 ) | d;

struct data {
	int id;
	vec4 position;
	float tmpx;
	float direction;	// an angle in the x:z -plane
	float fw_angle;
	float susp_angle;
	float F_centripetal;
	float velocity;
};

int server_createUDPSocket(unsigned int port);
int server_receive_packets();

int server_start(unsigned short int port);
int server_send_packet(unsigned char *data, size_t len, struct client* c);
int server_remove_client(int id);
struct client *get_client_by_id(int id);
int server_post_quit_message();
int server_post_peer_list();
int server_update_client_status(client* c);

sockaddr_in get_local_address();

int server_post_position_update(struct client *c);

#endif

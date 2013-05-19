#ifndef NET_PROTOCOL_H
#define NET_PROTOCOL_H

#include <cstring>
#include "net/net_client.h"

const int PROTOCOL_ID = 0xABACABAD;	// can't use a #define here; memcpy.
const unsigned short ID_SERVER = 0x0000; 

// command bytes

#define S_HANDSHAKE_OK (unsigned char) 0xE1
#define S_PUPD (unsigned char) 0xE2
#define S_PEER_LIST (unsigned char) 0xE3
#define S_PING (unsigned char) 0xE4
#define S_QUIT (unsigned char) 0xEF

#define C_HANDSHAKE (unsigned char) 0xF1
#define C_KEYSTATE (unsigned char) 0xF2
#define C_PONG (unsigned char) 0xF4
#define C_QUIT (unsigned char) 0xFF

void protocol_make_header(char *buffer, struct Client *client, unsigned short command_arg_mask);
std::string get_dot_notation_ipv4(struct sockaddr_in *saddr);


/*
The following protocol header for all datagram packets is used:

byte offset		content
0-4				(int) PROTOCOL_ID, packages which do not have this identifier will be dropped
4-6				(unsigned short) sender_id.	0x00 for server, 0xFF for unassigned client
6-10			(unsigned int) packet sequence number.
10-11			(unsigned char) command byte. see above
11-12			(unsigned char) command byte arg (optional). for example, S_PEER_LIST would have the number of peers here
12-limit		<varies by command>

*/

#endif
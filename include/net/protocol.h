#ifndef NET_PROTOCOL_H
#define NET_PROTOCOL_H

#include <cstring>
#include <cstdio>
#include <string>

const int PROTOCOL_ID = 0xABACABAD;	// can't use a #define here; memcpy.
const unsigned short ID_SERVER = 0x0000; 
const unsigned short ID_CLIENT_UNASSIGNED = 0xFFFF;

// command bytes

#define S_HANDSHAKE_OK (unsigned char) 0xE1
#define S_POSITION_UPDATE (unsigned char) 0xE2
#define S_PEER_LIST (unsigned char) 0xE3
#define S_CLIENT_CONNECT (unsigned char) 0xE4
#define S_CLIENT_DISCONNECT (unsigned char) 0xE5
#define S_PING (unsigned char) 0xE6
#define S_CLIENT_CHAT_MESSAGE (unsigned char) 0xE7
#define S_SHUTDOWN (unsigned char) 0xEF

#define C_HANDSHAKE (unsigned char) 0xF1
#define C_KEYSTATE (unsigned char) 0xF2
#define C_PONG (unsigned char) 0xF4
#define C_CHAT_MESSAGE (unsigned char) 0xF5
#define C_QUIT (unsigned char) 0xFF

#define C_KEYSTATE_UP (0x01 << 0)
#define C_KEYSTATE_DOWN (0x01 << 1)
#define C_KEYSTATE_LEFT (0x01 << 2)
#define C_KEYSTATE_RIGHT (0x01 << 3)

void protocol_make_header(char *buffer, unsigned short sender_id, unsigned int seq_number, unsigned short command_arg_mask);
void protocol_update_seq_number(char *buffer, unsigned int seq_number);
void buffer_print_raw(const char* buffer, size_t size);

std::string get_dot_notation_ipv4(const struct sockaddr_in *saddr);

typedef union {
		unsigned short us;
		unsigned char ch[2];
} command_arg_mask_union;

#define PTCL_HEADER_LENGTH 12

#define PTCL_ID_BYTERANGE  0, 4
#define PTCL_SENDER_ID_BYTERANGE 4, 6
#define PTCL_SEQ_NUMBER_BYTERANGE 6, 10
#define PTCL_CMD_ARG_BYTERANGE 10, 12
#define PTCL_DATAFIELD_BYTERANGE(SIZE) PTCL_HEADER_LENGTH, (PTCL_HEADER_LENGTH + (SIZE))



/*
The following protocol header for all datagram packets is used:

byte offset		content
0-4			(int) PROTOCOL_ID, packages which do not have this identifier will be dropped
4-6			(unsigned short) sender_id. ID_SERVER for server, ID_CLIENT_UNASSIGNED for unassigned client
6-10			(unsigned int) packet sequence number.
10-11			(unsigned char) command byte. see above
11-12			(unsigned char) command byte arg (mostly unused). for example, S_PEER_LIST would have the number of peers here
12-limit		<varies by command>

*/

#endif

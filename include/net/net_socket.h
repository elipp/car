#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <cstring>

#define PACKET_SIZE_MAX 256

class Socket {

	int fd;
	struct sockaddr_in my_addr;
	unsigned current_data_length;
	bool _bad;
	char packet_buffer[PACKET_SIZE_MAX];
public:

	void copy_from_packet_buffer(void *dst, size_t beg_offset, size_t end_offset);
	static int initialized();
	Socket(unsigned short port, int TYPE, bool blocking);
	static int initialize();	// winsock requires WSAStartup and all that stuff
	int send_data(char *data, size_t len, struct sockaddr_in *recipient);
	int receive_data(struct sockaddr_in *from);
	char get_packet_buffer_char(int index);
	void close();
	bool bad() const { return _bad; }
	Socket() { memset(this, 0, sizeof(*this)); }
	char *get_packet_buffer() { return packet_buffer; }
};

#endif
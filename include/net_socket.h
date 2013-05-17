#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include <winsock.h>

#define PACKET_SIZE_MAX 256

class Socket {
	int fd;
	struct sockaddr_in my_addr;
	char packet_buffer[PACKET_SIZE_MAX];
	unsigned current_data_length;
	bool _bad;
public:
	Socket(unsigned short port, int protocol);
	static int initialize();	// winsock requires WSAStartup and all that stuff
	int send_data(char *data, size_t len, struct sockaddr_in *recipient);
	int receive_data(struct sockaddr_in *from);
	int close();
	bool bad() const { return _bad; }
}

#endif
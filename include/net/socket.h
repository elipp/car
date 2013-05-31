#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <cstring>

#define PACKET_SIZE_MAX 256

#define _OUT

class Socket {

	int fd;
	struct sockaddr_in my_addr;
	bool _bad;
public:
#ifdef _WIN32
	static int initialized();
	static int initialize();	// winsock requires WSAStartup and all that stuff
#endif
	
	Socket(unsigned short port, int TYPE, bool blocking);
	int send_data(const struct sockaddr_in *recipient, const char* buffer, size_t len);
	int wait_for_incoming_data(int milliseconds);
	int receive_data(char *output_buffer, struct sockaddr_in _OUT *from);
	char get_packet_buffer_char(int index);
	void close();
	bool bad() const { return _bad; }
	Socket() { memset(this, 0, sizeof(*this)); }

};

#endif

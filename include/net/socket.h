#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <cstring>

#define PACKET_SIZE_MAX 256

class Socket {

	int fd;
	struct sockaddr_in my_addr;

	char inbound_packet_buffer[PACKET_SIZE_MAX];
	char outbound_packet_buffer[PACKET_SIZE_MAX];

	bool _bad;
	
	unsigned _current_data_length_in;
	unsigned _current_data_length_out;

public:
#ifdef _WIN32
	static int initialized();
	static int initialize();	// winsock requires WSAStartup and all that stuff
#endif
	
	void copy_from_inbound_buffer(void *dst, size_t beg_offset, size_t end_offset);
	void copy_to_outbound_buffer(const void *src, size_t src_size, size_t dest_offset);

	Socket(unsigned short port, int TYPE, bool blocking);

	int send_data(const struct sockaddr_in *recipient, size_t len);
	int receive_data(struct sockaddr_in *from);
	char get_packet_buffer_char(int index);
	void close();
	bool bad() const { return _bad; }
	Socket() { memset(this, 0, sizeof(*this)); }
	
	char *get_inbound_buffer() { return inbound_packet_buffer; }
	char *get_outbound_buffer() { return outbound_packet_buffer; }
		
	unsigned current_data_length_in() const { return _current_data_length_in; }
	unsigned current_data_length_out() const { return _current_data_length_out; };
};

#endif

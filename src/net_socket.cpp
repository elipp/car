#include "net_socket.h"
#include "common.h"

int Socket::initialize() {
	// windows specific; the rest don't need this
	int ret;
	WSAData wsaData;
	ret = WSAStartup(MAKEWORD(2,2), &wsaData);
	
	if (ret != 0) {
		logWindowOutput("WSAStartup failed.: %d\n", ret);
		return 0;
	}
	return 1;
}

Socket::Socket(unsigned short port, int protocol) {
	_bad = true;
	
	memset(packet_buffer, 0x0, PACKET_SIZE_MAX);
	current_data_length = 0;

	fd = socket(AF_INET, SOCK_DGRAM, protocol);
	if (fd <= 0) {
		logWindowOutput("Socket: Failed to create socket.\n");
	}
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons((unsigned short) port);

	if (bind(fd, (const sockaddr*)&my_addr, sizeof(struct sockaddr_in) ) < 0 ) {
		logWindowOutput("Socket: Failed to bind socket.\n");
		return;
	}
	u_long socket_mode = 1;
	if (ioctlsocket(fd, FIONBIO, &socket_mode) == SOCKET_ERROR) {
		logWindowOutput("Socket: failed to set non-blocking socket\n");
		return;
	}
	_bad = false;

}

int Socket::send_data(char *data, size_t len, struct sockaddr_in *recipient) {
	int sent_bytes = sendto(fd, (const char*)data, len, 0, (struct sockaddr*)recipient, sizeof(struct sockaddr));
	return sent_bytes;
}

int Socket::receive_data(struct sockaddr_in *from) {
	int from_length = sizeof(*from);
	
	memset(from, 0x0, from_length);
	
	int received_bytes = 
		recvfrom(fd, packet_buffer, PACKET_SIZE_MAX, 0, (struct sockaddr*)&from, &from_length);

	packet_buffer[received_bytes] = '\0';
	
	return received_bytes;
}


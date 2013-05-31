#include "net/socket.h"
#include "common.h"

#ifdef _WIN32
static int _initialized = 0;

int Socket::initialized() {
	return _initialized;
}

int Socket::initialize() {
	// windows specific; the rest don't need this
	if (initialized()) { return 1; }
	int ret;
	WSAData wsaData;
	ret = WSAStartup(MAKEWORD(2,2), &wsaData);
	
	if (ret != 0) {
		fprintf(stderr, "WSAStartup failed.: %d\n", ret);
		return 0;
	}
	_initialized = 1;
	return 1;
}
#endif

Socket::Socket(unsigned short port, int TYPE, bool blocking) {
	_bad = true;

	fd = socket(AF_INET, TYPE, 0);
	if (fd <= 0) {
		fprintf(stderr, "Socket: Failed to create socket (error code %d).\n", WSAGetLastError());
		return;
	}
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons((unsigned short) port);

	while (bind(fd, (const sockaddr*)&my_addr, sizeof(struct sockaddr_in) ) < 0 ) {
		fprintf(stderr, "Socket: bind request at port %u failed, trying port %u.\n", port, ++port);
		my_addr.sin_port = htons((unsigned short) port);
	}
	
	u_long socket_mode = 1;

	if (!blocking) {
		if (ioctlsocket(fd, FIONBIO, &socket_mode) == SOCKET_ERROR) {
			fprintf(stderr, "Socket: failed to set non-blocking socket\n");
			return;
		}
		fprintf(stderr, "Socket: using non-blocking socket.\n");
	}
	else { 
		fprintf(stderr, "Socket: using blocking socket.\n");
	}
	_bad = false;

}

int Socket::wait_for_incoming_data(int milliseconds) {
	
	fd_set read_fds, write_fds, except_fds;
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_ZERO(&except_fds);
	FD_SET(fd, &read_fds);

	// Set timeout to 1.0 seconds
	struct timeval timeout;

	timeout.tv_sec = milliseconds/1000;
	int modulo = milliseconds%1000;
	timeout.tv_usec = modulo*1000;
	
	return select(fd + 1, &read_fds, &write_fds, &except_fds, &timeout);
	
}


int Socket::send_data(const struct sockaddr_in *recipient, const char* buffer, size_t len) {
	int bytes = sendto(fd, (const char*)buffer, len, 0, (struct sockaddr*)recipient, sizeof(struct sockaddr));
	return bytes;
}

int Socket::receive_data(char *buffer, struct sockaddr_in _OUT *out_from) {
	
	int from_length = sizeof(struct sockaddr_in);
	memset(out_from, 0x0, from_length);	// probably not necessary
	
	int bytes = recvfrom(fd, buffer, PACKET_SIZE_MAX, 0, (struct sockaddr*)out_from, &from_length);

	buffer[max(bytes, 0)] = '\0';
	
	return bytes;
}


void Socket::close() {
	closesocket(fd);
}


#include "net/net_socket.h"
#include "common.h"

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

Socket::Socket(unsigned short port, int TYPE, bool blocking) {
	_bad = true;
	
	memset(packet_buffer, 0x0, PACKET_SIZE_MAX);
	current_data_length = 0;

	fd = socket(AF_INET, TYPE, 0);
	if (fd <= 0) {
		fprintf(stderr, "Socket: Failed to create socket (TYPE: %d).\n", WSAGetLastError());
		return;
	}
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons((unsigned short) port);

	if (bind(fd, (const sockaddr*)&my_addr, sizeof(struct sockaddr_in) ) < 0 ) {
		fprintf(stderr, "Socket: Failed to bind socket.\n");
		return;
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

int Socket::send_data(struct sockaddr_in *recipient, size_t len) {
	int sent_bytes = sendto(fd, (const char*)outbound_buffer, len, 0, (struct sockaddr*)recipient, sizeof(struct sockaddr));
	return sent_bytes;
}

int Socket::receive_data(struct sockaddr_in *from) {
	int from_length = sizeof(struct sockaddr_in);
	
	memset(from, 0x0, from_length);
	
	current_data_length = 
		recvfrom(fd, packet_buffer, PACKET_SIZE_MAX, 0, (struct sockaddr*)from, &from_length);

	packet_buffer[current_data_length] = '\0';
	
	return current_data_length;
}

void Socket::close() {
	closesocket(fd);
}


void Socket::copy_from_packet_buffer(void *dst, size_t beg_offset, size_t end_offset) {
	size_t size = end_offset-beg_offset;
	memcpy(dst, packet_buffer + beg_offset, size); 
}

void Socket::copy_to_outbound_buffer(void *src, size_t src_size, size_t dest_offset) {
	if (dest_offset + src_size > PACKET_SIZE_MAX) {
		fprintf(stderr, "copy_to_outbound_buffer(): warning: size of data to be copied > PACKET_SIZE_MAX. Truncating.\n");
		src_size = PACKET_SIZE_MAX - dest_offset - 1;
	}
	memcpy(outbound_buffer + dest_offset, src, src_size);
}

char Socket::get_packet_buffer_char(int index) {
	if (index > PACKET_SIZE_MAX) {
		index = PACKET_SIZE_MAX-1;
	}
	return packet_buffer[index];
}

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
	
	memset(inbound_packet_buffer, 0x0, PACKET_SIZE_MAX);
	memset(outbound_packet_buffer, 0x0, PACKET_SIZE_MAX);
	_current_data_length_in = 0;
	_current_data_length_out = 0;

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

int Socket::send_data(const struct sockaddr_in *recipient, size_t len) {
	int sent_bytes = sendto(fd, (const char*)outbound_packet_buffer, len, 0, (struct sockaddr*)recipient, sizeof(struct sockaddr));
	_current_data_length_out = len;
	return sent_bytes;
}

int Socket::receive_data(struct sockaddr_in *from) {
	int from_length = sizeof(struct sockaddr_in);
	
	memset(from, 0x0, from_length);
	
	int bytes = 
		recvfrom(fd, inbound_packet_buffer, PACKET_SIZE_MAX, 0, (struct sockaddr*)from, &from_length);

	_current_data_length_in = bytes > 0 ? bytes : 0;
	inbound_packet_buffer[_current_data_length_in] = '\0';
	
	return bytes;
}


void Socket::close() {
	closesocket(fd);
}


void Socket::copy_from_inbound_buffer(void *dst, size_t beg_offset, size_t end_offset) {
	size_t size = end_offset-beg_offset;
	memcpy(dst, inbound_packet_buffer + beg_offset, size); 
}

void Socket::copy_to_outbound_buffer(const void *src, size_t src_size, size_t dest_offset) {
	if (dest_offset + src_size > PACKET_SIZE_MAX) {
		fprintf(stderr, "copy_to_outbound_buffer(): warning: size of data to be copied > PACKET_SIZE_MAX. Truncating.\n");
		src_size = PACKET_SIZE_MAX - dest_offset - 1;
	}
	memcpy(outbound_packet_buffer + dest_offset, src, src_size);
}

char Socket::get_packet_buffer_char(int index) {
	if (index > PACKET_SIZE_MAX) {
		index = PACKET_SIZE_MAX-1;
	}
	return inbound_packet_buffer[index];
}

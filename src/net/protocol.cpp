#include "net/protocol.h"

#include "net/socket.h"

#include "common.h"


#define IPBUFSIZE 32
std::string get_dot_notation_ipv4(const struct sockaddr_in *saddr) {
		char ip_buf[IPBUFSIZE];
		IN_ADDR sin_addr = saddr->sin_addr;
		inet_ntop(saddr->sin_family, &sin_addr, ip_buf, IPBUFSIZE);
		ip_buf[IPBUFSIZE-1] = '\0';
		return std::string(ip_buf);
}

int protocol_copy_header(char *buffer, const PTCLHEADERDATA *header) {
	memcpy(buffer, header, sizeof(*header));
	return sizeof(*header);
}

void protocol_update_seq_number(char *buffer, unsigned int seq_number) {
	memcpy(buffer+4, &seq_number, sizeof(seq_number)); 
}

void protocol_get_header_data(const char* buffer, PTCLHEADERDATA *out_data) {
	memcpy(out_data, buffer, sizeof(PTCLHEADERDATA));
}

void buffer_print_raw(const char* buffer, size_t size) {
	fprintf(stderr, "buffer_print_raw: printing %u bytes of data.\n", size);
	for (size_t i = 0; i < size; ++i) {
		if ((unsigned char)buffer[i] < 0x7F && (unsigned char)buffer[i] > 0x1F) {
			fprintf(stderr, "%c  ", buffer[i]);
		}
		else {
			fprintf(stderr, "%02x ", (unsigned char)buffer[i]);
		}
		if (i % 16 == 15) { fprintf(stderr, "\n"); }
	}
	fprintf(stderr, "\n");
}

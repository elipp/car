#include "net/protocol.h"

#include "net/socket.h"

#include "common.h"

static size_t accum_offset = 0;

static void accum_reset() { accum_offset = 0; }

static inline void accum_write(char *buffer, const void *data, size_t size) {
	memcpy(buffer + accum_offset, data, size);
	accum_offset += size;
}

void protocol_make_header(char* buffer, unsigned short sender_id, unsigned int seq_number, unsigned short command_arg_mask) {
	accum_reset();

	accum_write(buffer, (const void*)&PROTOCOL_ID, sizeof(PROTOCOL_ID));
	accum_write(buffer, (const void*)&sender_id, sizeof(sender_id));
	accum_write(buffer, (const void*)&seq_number, sizeof(seq_number));
	accum_write(buffer, (const void*)&command_arg_mask, sizeof(command_arg_mask));
}

#define IPBUFSIZE 32

std::string get_dot_notation_ipv4(const struct sockaddr_in *saddr) {
		char ip_buf[IPBUFSIZE];
		IN_ADDR sin_addr = saddr->sin_addr;
		inet_ntop(saddr->sin_family, &sin_addr, ip_buf, IPBUFSIZE);
		ip_buf[IPBUFSIZE-1] = '\0';
		return std::string(ip_buf);
}

void protocol_update_seq_number(char *buffer, unsigned int seq_number) {
	memcpy(buffer+6, &seq_number, sizeof(seq_number)); 
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

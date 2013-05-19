#include "net/net_protocol.h"

static size_t accum_offset = 0;

static void accum_reset() { accum_offset = 0; }

static inline void accum_write(char *buffer, const void *data, size_t size) {
	memcpy(buffer + accum_offset, data, size);
	accum_offset += size;
}

void protocol_make_header(char* buffer, struct Client *client, unsigned short command_arg_mask) {
	accum_reset();

	accum_write(buffer, (const void*)&PROTOCOL_ID, sizeof(PROTOCOL_ID));
	unsigned short id = client->id;
	accum_write(buffer, (const void*)&id, sizeof(id));
	accum_write(buffer, (const void*)&client->seq_number, sizeof(client->seq_number));
	accum_write(buffer, (const void*)&command_arg_mask, sizeof(command_arg_mask));
}

#define IPBUFSIZE 32

std::string get_dot_notation_ipv4(struct sockaddr_in *saddr) {
		char ip_buf[IPBUFSIZE];
		inet_ntop(saddr->sin_family, &saddr->sin_addr, ip_buf, IPBUFSIZE);
		ip_buf[IPBUFSIZE-1] = '\0';
		return std::string(ip_buf);
}
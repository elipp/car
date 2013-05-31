#include "net/taskthread.h"
#include "net/protocol.h"

int NetTaskThread::copy_to_buffer(const void* src, size_t length, size_t offset) {
	int diff = offset + length - PACKET_SIZE_MAX;
	if (diff >= 0) { 
		fprintf(stderr, "WARNING! copy_to_buffer: offset + length > PACKET_SIZE_MAX - PTCL_HEADER_LENGTH. errors inbound!\n");
		length = diff;
	}
	memcpy(buffer + offset, src, length);
	return length;
}

void NetTaskThread::copy_from_buffer(void *target, size_t length, size_t offset) {
	memcpy(target, buffer+offset, length);
}
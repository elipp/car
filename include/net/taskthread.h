#ifndef TASKTHREAD_H
#define TASKTHREAD_H

#include "net/socket.h"
#include <thread>

typedef void (*NTTCALLBACK)(void);

// this is not quite the interface i wanted though :P
class NetTaskThread {
	std::thread thread_handle;
	NTTCALLBACK task_callback;
	int _running;
public:
	char buffer[PACKET_SIZE_MAX];

	int copy_to_buffer(const void* src, size_t length, size_t offset);
	void copy_from_buffer(void* target, size_t length, size_t offset);
	
	void start() { 
		_running = 1;
		thread_handle = std::thread(task_callback); 
	}
	void stop() { 
		_running = 0; 
		if (thread_handle.joinable()) { thread_handle.join(); }
	}

	int running() const { return _running; }
	
	NetTaskThread() {};

	NetTaskThread(NTTCALLBACK callback) {
		memset(buffer, 0x0, PACKET_SIZE_MAX);
		task_callback = callback;
		_running = 0;
	}
};


#endif
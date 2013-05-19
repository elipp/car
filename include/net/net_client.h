#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <string>
#include <cstring>
#include <unordered_map>

#include "net_protocol.h"
#include "net_socket.h"
#include "common.h"

struct Client {
	struct sockaddr_in address;
	const char* ip_string;
	const char* name;
	unsigned short id;
	Car car;
	unsigned char keystate;
	unsigned seq_number;

	Client() { memset(this, 0x0, sizeof(struct Client)); }
	
	~Client() { 
		if(ip_string != NULL) { free((void*)ip_string); }
		if(name != NULL) { free((void*)name); }
	}
};

class LocalClient {
public:
	static Socket socket;
	static struct Client client;
	static struct sockaddr_in remote_sockaddr;
	static std::unordered_map<unsigned short, struct Client> peers;

	static int init(const std::string &name, const std::string &remote_ip, unsigned short int port);
	static int handshake();
	static void reconstruct_peer_list();
private:
	LocalClient() {}
};

#endif
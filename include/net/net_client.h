#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <string>
#include <cstring>
#include <unordered_map>
#include <thread>

#include "net_protocol.h"
#include "net_socket.h"
#include "common.h"

struct Client {
	struct sockaddr_in address;
	std::string ip_string;
	std::string name;
	unsigned short id;
	Car car;
	unsigned char keystate;
	unsigned seq_number;
	unsigned active_ping_seq_number;
	_timer ping_timer;

	Client() { 
		memset(&address, 0x0, sizeof(address));
		id = 0;
		memset(&car, 0x0, sizeof(car));
		keystate = 0x0;
		seq_number = 0x0;
		active_ping_seq_number = 0x0;
	}
};

class LocalClient {
public:
	static std::thread client_thread;
	static Socket socket;
	static struct Client client;
	static struct sockaddr_in remote_sockaddr;
	static std::unordered_map<unsigned short, struct Client> peers;

	static int init(const std::string &name, const std::string &remote_ip, unsigned short int port);
	static int handshake();
	static void listen();
	static void reconstruct_peer_list();
	static void handle_current_packet();
	static void post_quit_message();
	static void pong(unsigned remote_seq_number);
	static void quit();
	static int send_current_data(size_t size); // use a wrapper like this in order to increment seq_numbers
private:
	LocalClient() {}
};

#endif
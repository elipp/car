#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <string>
#include <cstring>
#include <unordered_map>
#include <thread>
#include <mutex>

#include "net/protocol.h"
#include "net/socket.h"
#include "common.h"

extern const vec4 colors[];

struct peer_info_t {
	unsigned short id;
	std::string name;
	std::string ip_string;
	uint8_t color;
	peer_info_t(unsigned short _id, const std::string &_name, const std::string &_ip_string, uint8_t _color) 
		: id(_id), name(_name), ip_string(_ip_string), color(_color) {}
	peer_info_t() {};
	bool operator==(const struct peer_info_t &b) const { 
		const struct peer_info_t &a = *this;
		return ((a.id == b.id) && (a.name == b.name) && (a.ip_string == b.ip_string));
	}
};

struct Peer {
	peer_info_t info;
	Car car;
	Peer(unsigned short _id, const std::string &_name, const std::string &_ip_string, uint8_t _color) {
		info = peer_info_t(_id, _name, _ip_string, _color);
		memset(&car, 0x0, sizeof(car));
	}
};

struct Client {
	struct sockaddr_in address;
	struct peer_info_t info;
	Car car;
	unsigned char keystate;
	unsigned seq_number;
	unsigned active_ping_seq_number;
	_timer ping_timer;

	Client() {
		
		memset(&address, 0x0, sizeof(address));
		info = struct peer_info_t(ID_CLIENT_UNASSIGNED, "Anonymous", "1.1.1.1", 0);
		memset(&car, 0x0, sizeof(car));
		keystate = 0x0;
		seq_number = 0x0;
		active_ping_seq_number = 0x0;
	}
};


struct mutexed_peer_map {
	std::unordered_map<unsigned short, struct Peer> map;
	std::mutex mutex;
};

class LocalClient {
	static std::thread keystate_thread;
	static std::thread listen_thread;
	static Socket socket;
	static struct Client client;
	static struct sockaddr_in remote_sockaddr;
	static std::unordered_map<unsigned short, struct Peer> peers;
	static int _connected;	// "connected" :P
	
	static int _listening;

	static void start_thread();	// thread wrapper function
	static bool _bad;
	static void handle_current_packet();
	static void pong(unsigned remote_seq_number);
	static void listen();
	static int handshake();
	static void construct_peer_list();
	static void post_quit_message();
	static void post_keystate();
	static void update_positions();
	static void keystate_loop();
	static int send_data_to_server(size_t size);
	static int send_data_to_server(const char* buffer, size_t size); 

public:
	static const std::unordered_map<unsigned short, struct Peer> get_peers() { return peers; }
	static int init(const std::string &name, const std::string &remote_ip, unsigned short int port);
	static void quit();
	static void update_keystate(const bool * keys);

private:
	LocalClient() {}
};

#endif

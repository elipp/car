#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <string>
#include <cstring>
#include <unordered_map>
#include <thread>
#include <mutex>

#include "net/protocol.h"
#include "net/socket.h"
#include "net/taskthread.h"

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

	static NetTaskThread parent;	// hosts all the other threads.

	static Socket socket;
	static struct Client client;
	static struct sockaddr_in remote_sockaddr;
	
	static std::unordered_map<unsigned short, struct Peer> peers;
	
	static unsigned short port;
	static int _connected;
	static bool _failure;
	static bool _received_shutdown;

	static class Listen {
		NetTaskThread thread;
		void handle_current_packet();
		void pong(unsigned remote_seq_number);


		void construct_peer_list();
		void post_quit_message();
		void post_keystate();
		void update_positions();
	public:
		void listen();
		Listen() : thread(listen_task) {}

		void start() { thread.start(); }
		void stop() { thread.stop(); }

	} Listener;
	static void listen_task();

	static class Keystate {
		NetTaskThread thread;
		void update_keystate(const bool * keys);
		void post_keystate();
	public:
		void keystate_loop();
		Keystate() : thread(keystate_task) { }
		void start() { thread.start(); }
		void stop() { thread.stop(); }

	} KeystateManager;
	static void keystate_task();

	static int send_data_to_server(const char* buffer, size_t size); 
	static void send_chat_message(const std::string &msg);
	
	static int handshake();
	
public:
	static int connected() { return _connected; }
	static void connect();
	static void disconnect();

	static bool received_shutdown() { return _received_shutdown; }

	static int start(const std::string &ip_port_string);
	static void stop();
	static void stop_parent_thread();

	static void set_nick(const std::string &nick);
	static void parse_user_input(const std::string s);
	static const std::unordered_map<unsigned short, struct Peer> get_peers() { return peers; }
	static void quit();
private:
	LocalClient() {}
};

#endif

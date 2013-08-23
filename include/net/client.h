#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <string>
#include <cstring>
#include <unordered_map>
#include <thread>
#include <mutex>

#include "net/client_funcs.h"
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

typedef std::unordered_map<unsigned short, struct Peer>::iterator peer_iter;

class mutexed_peer_map {
	
	std::unordered_map<unsigned short, struct Peer> map;
	std::mutex mutex;
public:
	peer_iter begin() { return map.begin(); }
	peer_iter end() { return map.end(); }
	peer_iter find(unsigned short key) { return map.find(key); }
	size_t num_peers() { return map.size(); }
	
	std::unordered_map<unsigned short, struct Peer> get_map_copy() { return map; }
	void insert(std::pair<unsigned short, struct Peer> &p) { 
		mutex.lock();
		map.insert(p);
		mutex.unlock();
	}
	void erase(unsigned short id) {
		mutex.lock();
		map.erase(id);
		mutex.unlock();
	}
	void update_peer_data(peer_iter &iter, const void *src, size_t src_size) {
		// the iter part has already been checked for errors, so just carry on 
		mutex.lock();
		memcpy(&iter->second.car, src, src_size);
		mutex.unlock();
	}
	void clear() { 
		mutex.lock();
		map.clear(); 
		mutex.unlock();
	}
};

class LocalClient {
	
	static Socket socket;
	static struct Client client;
	static struct sockaddr_in remote_sockaddr;
	
	static std::string preferred_name;

	static mutexed_peer_map peers;
	
	static unsigned short port;
	static int _connected;
	static bool _shutdown_requested;
	
	static _timer posupd_timer;

	static double _latency;

	static unsigned latest_posupd_seq_number;

	static class Listen : public NetTaskThread { 
		void handle_current_packet();
		void construct_peer_list();
		void post_quit_message();
		void update_positions();
	public:
		void task();
		Listen(NTTCALLBACK callback) : NetTaskThread(callback) {};
	} Listener;

	static class PingManager : public NetTaskThread {
		void ping();	
		_timer timer;
	public:
		unsigned _latest_ping_seq_number;
		double time_since_last_ping_ms() { return timer.get_ms(); }
		void task();
		PingManager(NTTCALLBACK callback) : NetTaskThread(callback) {};
	} Ping;

	static class Keystate : public NetTaskThread {
		void update_keystate(const bool *keys);
		void post_keystate();
	public:
		void task();
		Keystate(NTTCALLBACK callback) : NetTaskThread(callback) {};
	} KeystateManager;

	static int send_data_to_server(const char* buffer, size_t size); 
	static void send_chat_message(const std::string &msg);
	
	static int handshake();
	
public:
	
	static inline double time_since_last_posupd_ms() { return posupd_timer.get_ms(); }

	static void request_shutdown() { _connected = 0; _shutdown_requested = true; }

	static void interpolate_positions();
	static int connected() { return _connected; }
	static int connect(const std::string &ip_port_string);
	static void disconnect();

	static bool shutdown_requested() { return _shutdown_requested; }

	static void stop();
	
	static unsigned short id() { return client.info.id; }

	static size_t num_peers(){ return peers.num_peers(); }
	static void set_name(const std::string &nick);
	static void parse_user_input(const std::string s);
	static const std::unordered_map<unsigned short, struct Peer> get_peers() { return peers.get_map_copy(); }
	static void quit();

	static double latency() { return _latency; }

private:
	LocalClient() {}
};

#endif

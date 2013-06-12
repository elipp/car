#ifndef NET_SERVER_H
#define NET_SERVER_H

#include <thread>
#include <unordered_map>
#include "protocol.h"
#include "socket.h"
#include "net/taskthread.h"

typedef std::unordered_map<unsigned short, struct Client> id_client_map;
typedef std::pair<unsigned short, struct Client> id_client_pair;

#define POSITION_UPDATE_GRANULARITY_MS 16.666*2
#define POSITION_UPDATE_DT_COEFF (POSITION_UPDATE_GRANULARITY_MS/16.666)

#define _NETTASKTHREAD_CALLBACK

// posupd constants :P

const float turning_modifier_forward = 0.55;
const float turning_modifier_reverse = 1.2*turning_modifier_forward;

const float accel_modifier = 0.012*POSITION_UPDATE_DT_COEFF;
const float brake_modifier = 0.8*accel_modifier;

inline float turn_velocity_coeff(float vel) {
	return 1.5*pow(8, -fabs(vel));
}

const float velocity_dissipation = pow(0.962, POSITION_UPDATE_DT_COEFF);

const float tmpx_limit = 0.68;
const float tmpx_dissipation = pow(0.95, POSITION_UPDATE_DT_COEFF);
const float tmpx_modifier = 0.22*POSITION_UPDATE_DT_COEFF;


class Server {
	
	static id_client_map clients;
	static unsigned num_clients;
	static unsigned seq_number;

	static Socket socket;
	
	static int _running;

	// these should be grouped and more strictly "bound" in a way other than just grouping them together at the source code level..
	static class Listen : public NetTaskThread {
		void handle_current_packet(struct sockaddr_in *from);
		void distribute_chat_message(const std::string &msg, const unsigned short sender_id);
		void post_peer_list();
		void post_client_connect(const struct Client &newclient);
		void post_client_disconnect(unsigned short id);
		void handshake(struct Client *client);
		unsigned short add_client(struct sockaddr_in *newclient_saddr, const std::string &name);
		void broadcast_shutdown_message();
	public:
		void task();
		Listen(NTTCALLBACK callback) : NetTaskThread(callback) {};

	} Listener;
	
	static class Ping : public NetTaskThread {
		void ping_client(struct Client &c);
	public:
		void task();
		Ping(NTTCALLBACK callback) : NetTaskThread(callback) {};
	} PingManager;


	static class GameState : public NetTaskThread {
		inline void calculate_state_client(struct Client &c);
		void broadcast_state();
	public:
		void task();
		GameState(NTTCALLBACK callback) : NetTaskThread(callback) {};
	} GameStateManager;
	

	static void increment_client_seq_number(struct Client &client);
	static int send_data_to_client(struct Client &client, const char* buffer, size_t data_size);
	static void send_data_to_all(char* data, size_t size);

	static id_client_map::iterator remove_client(id_client_map::iterator &iter);

public:
	static unsigned short get_port() { return socket.get_port(); }
	static int running() { return _running; }	
	static int init(unsigned short port);
	static void shutdown();
private:
	Server();
};


#endif

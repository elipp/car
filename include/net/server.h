#ifndef NET_SERVER_H
#define NET_SERVER_H

#include <thread>
#include <unordered_map>
#include "protocol.h"
#include "socket.h"
#include "client.h"
#include "net/taskthread.h"

typedef std::unordered_map<unsigned short, struct Client> id_client_map;
typedef std::pair<unsigned short, struct Client> id_client_pair;

#define _NETTASKTHREAD_CALLBACK

class Server {
	
	static id_client_map clients;
	static unsigned num_clients;
	static unsigned seq_number;

	static Socket socket;
	
	static int _running;

	// these should be grouped and more strictly "bound" in a way other than just grouping them together at the source code level..
	static class Listen {
		NetTaskThread thread;
		void handle_current_packet(struct sockaddr_in *from);
		void distribute_chat_message(const std::string &msg, const unsigned short sender_id);
		void post_peer_list();
		void post_client_connect(const struct Client &newclient);
		void post_client_disconnect(unsigned short id);
		void handshake(struct Client *client);
		unsigned short add_client(struct sockaddr_in *newclient_saddr, const std::string &name);
		void broadcast_shutdown_message();
	public:
		void listen_loop();
		Listen() : thread(listen_task) {};
		void start() { thread.start(); }
		void stop() { thread.stop(); }
	} Listener;
	
	static void _NETTASKTHREAD_CALLBACK listen_task();
	
	// horrible code duplication :(
	static class Ping {
		NetTaskThread thread;
		void ping_client(struct Client &c);
	public:
		void ping_loop();
		Ping() : thread(ping_task) {};
		void start() { thread.start(); }
		void stop() { thread.stop(); }
	} PingManager;
	static _NETTASKTHREAD_CALLBACK void ping_task();

	static class GameState {

		NetTaskThread thread;
		void calculate_state();
		inline void calculate_state_client(struct Client &c);
		void broadcast_state();
	public:
		GameState() : thread(state_task) {};
		void state_loop();
		void start() { thread.start(); }
		void stop() { thread.stop(); }
	} GameStateManager;
	
	static _NETTASKTHREAD_CALLBACK void state_task();

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

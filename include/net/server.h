#ifndef NET_SERVER_H
#define NET_SERVER_H

#include <thread>
#include <unordered_map>
#include "protocol.h"
#include "socket.h"
#include "client.h"

typedef std::unordered_map<unsigned short, struct Client> id_client_map;
typedef std::pair<unsigned short, struct Client> id_client_pair;

class Server {
	static std::thread ping_thread;
	static std::thread position_thread;
	static std::thread listen_thread;
	static id_client_map clients;
	static unsigned num_clients;
	static unsigned seq_number;
	static Socket socket;
	static void ping_client(struct Client &c);
	static void ping_clients();
	static void calculate_state();
	static void broadcast_state();
	static void handle_current_packet(struct sockaddr_in *from);
	static int send_data_to_client(struct Client &client, const char* buffer, size_t data_size);
	static int send_data_to_client(struct Client &client, size_t data_size);
	static void increment_client_seq_number(struct Client &client);
	static void send_data_to_all(size_t size);
	static void send_data_to_all(char* data, size_t size);

	static void distribute_chat_message(const std::string &msg, const unsigned short sender_id);

	static void post_peer_list();
	static void post_client_connect(const struct Client &newclient);
	static void post_client_disconnect(unsigned short id);

	static void listen();
	static void handshake(struct Client *client);

	static unsigned short add_client(struct sockaddr_in *newclient_saddr, const std::string &name);
	static id_client_map::iterator remove_client(id_client_map::iterator &iter);

	static void broadcast_shutdown_message();
public:

	static int init(unsigned short port);
	static void shutdown();
private:
	Server();
};


#endif

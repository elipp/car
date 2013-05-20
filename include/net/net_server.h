#ifndef NET_SERVER_H
#define NET_SERVER_H

#include <thread>
#include <unordered_map>
#include "net_protocol.h"
#include "net_socket.h"
#include "net_client.h"

typedef std::unordered_map<unsigned short, struct Client> id_client_map;
typedef std::pair<unsigned short, struct Client> id_client_pair;

class Server {
	static std::thread listen_thread;
	static id_client_map clients;
	static unsigned num_clients;
	static unsigned seq_number;
public:
	
	static int init(unsigned short port);

	static Socket socket;
	static void listen();
	static void handshake(struct Client *client);
	static unsigned short add_client(struct sockaddr_in *newclient_saddr, const std::string &name);
	static void remove_client(int id);
	static id_client_map::iterator get_client_by_id(unsigned short id);
	static void ping_clients();
	static void broadcast_state();
	static void handle_current_packet(struct sockaddr_in *from);
	static int send_data_to_client(struct Client &client, size_t data_size);
	static void post_peer_list();

	static void post_quit_and_cleanup();
private:
	Server();
};


#endif
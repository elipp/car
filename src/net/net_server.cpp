#include "net/net_server.h"

std::thread Server::listen_thread;
std::unordered_map<unsigned short, struct Client> Server::clients;
unsigned Server::num_clients = 0;
Socket Server::socket;

#define IPBUFSIZE 32

static int listening = 0;

static inline void extract_from_buffer(void *dest, const void* source, size_t size) {
	memcpy(dest, source, size);
}

void Server::listen() {
	listening = 1;
	while(listening) {
		sockaddr_in from;
		int bytes = socket.receive_data(&from);
		if (bytes > 0) {
			char ip_buf[IPBUFSIZE];
			inet_ntop(from.sin_family, &from.sin_addr, ip_buf, IPBUFSIZE);
			ip_buf[IPBUFSIZE-1] = '\0';
			/*logWindowOutput("received %d bytes from %d.%d.%d.%d\n", bytes,
			 int(from.sin_addr.s_addr&0xFF),
			 int((from.sin_addr.s_addr&0xFF00)>>8),
			 int((from.sin_addr.s_addr&0xFF0000)>>16),
			 int((from.sin_addr.s_addr&0xFF000000)>>24)); */
			logWindowOutput("Received %d bytes from %s\n", bytes, ip_buf);
			Server::handle_current_packet(&from);
		}
	}
	logWindowOutput("Stopping listen.\n");
}

int Server::init(unsigned short port) {
	if(!Socket::initialized()) { Socket::initialize(); }
	socket = Socket(port, SOCK_DGRAM, false);	// for UDP and blocking
	if (socket.bad()) { logWindowOutput("Server::init, socket init error.\n"); return 0; }
	listen_thread = std::thread(listen);	// start listening thread
	return 1;
}

void Server::post_quit_and_cleanup() {
	// post quit messages to all clients
	listening = 0;
	listen_thread.join();
	socket.close();
	WSACleanup();
}

void Server::handle_current_packet(struct sockaddr_in *from) {
	int protocol_id;
	socket.copy_from_packet_buffer(&protocol_id, 0, 4);
	if (protocol_id != PROTOCOL_ID) {
		logWindowOutput("dropping packet. Reason: protocol_id mismatch (%d)\n", protocol_id);
	}
	unsigned short client_id;
	socket.copy_from_packet_buffer(&client_id, 4, 6);

	unsigned int seq;
	socket.copy_from_packet_buffer(&seq, 6, 10);

	unsigned short cmdbyte_arg_mask;
	socket.copy_from_packet_buffer(&cmdbyte_arg_mask, 10, 12);
	char cmdbyte = cmdbyte_arg_mask & 0x00FF;
	char cmdbyte_arg = (cmdbyte_arg_mask >> 8) & 0x00FF;

	if (cmdbyte == C_HANDSHAKE) {
		std::string name_string(socket.get_packet_buffer() + 12);
		unsigned short new_id = add_client(from, name_string);
		id_client_map::iterator newcl = get_client_by_id(new_id);
		if(newcl != clients.end()) {
			handshake(newcl->second);
		}
		else {
			logWindowOutput("server: internal error @ handshake (this shouldn't happen!).\n");
		}
	}
	else if (cmdbyte == C_QUIT) {

	}

}

unsigned short Server::add_client(struct sockaddr_in *newclient_saddr, const std::string &name) {
	++num_clients;
	
	struct Client newclient;
	newclient.address = *newclient_saddr;
	newclient.id = num_clients;
	newclient.name = _strdup(name.c_str());
	
	char ip_buf[IPBUFSIZE];
	inet_ntop(newclient_saddr->sin_family, newclient_saddr, ip_buf, IPBUFSIZE); 
	ip_buf[IPBUFSIZE-1] = '\0';
	newclient.ip_string = _strdup(ip_buf);
	
	clients.insert(std::pair<unsigned short, struct Client>(newclient.id, newclient));

	return newclient.id;
}

std::unordered_map<unsigned short, struct Client>::iterator Server::get_client_by_id(unsigned short id) {
	return clients.find(id);
}

void Server::handshake(struct Client client) {
	protocol_make_header(socket.get_packet_buffer,
}
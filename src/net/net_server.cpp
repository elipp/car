#include "net/net_server.h"

unsigned Server::seq_number = 0;
std::thread Server::listen_thread;
std::unordered_map<unsigned short, struct Client> Server::clients;
unsigned Server::num_clients = 0;
Socket Server::socket;

static int listening = 0;

static inline void extract_from_buffer(void *dest, const void* source, size_t size) {
	memcpy(dest, source, size);
}

void Server::listen() {
	listening = 1;
	_timer _ping_timer;
	_ping_timer.init();
	_ping_timer.begin();
	
	while(listening) {
		sockaddr_in from;
		int bytes = socket.receive_data(&from);
		if (bytes > 0) {
			std::string ip = get_dot_notation_ipv4(&from);
			fprintf(stderr, "Received %d bytes from %s.\n", bytes, ip.c_str());
			Server::handle_current_packet(&from);
		}
		if (_ping_timer.get_ms() > 1000) {
			ping_clients();
			_ping_timer.begin();
		}
	}
	fprintf(stderr, "Stopping listen.\n");
}

int Server::init(unsigned short port) {
	Socket::initialize();
	socket = Socket(port, SOCK_DGRAM, false);	// for UDP and non-blocking
	if (socket.bad()) { fprintf(stderr, "Server::init, socket init error.\n"); return 0; }
	listen_thread = std::thread(listen);	// start listening thread
	return 1;
}

void Server::shutdown() {
	// post quit messages to all clients
	listening = 0;
	broadcast_shutdown_message();
	if (listen_thread.joinable()) { listen_thread.join(); }
	socket.close();
	WSACleanup();
}


void Server::handle_current_packet(struct sockaddr_in *from) {
	int protocol_id;
	socket.copy_from_packet_buffer(&protocol_id, PROTOCOL_ID_LIMITS);
	if (protocol_id != PROTOCOL_ID) {
		fprintf(stderr, "dropping packet. Reason: protocol_id mismatch (%d)\n", protocol_id);
		return;
	}

	unsigned short client_id;
	socket.copy_from_packet_buffer(&client_id, SENDER_ID_LIMITS);
	
	if (client_id != 0xFF) {
		id_client_map::iterator it = clients.find(client_id);
		if (it == clients.end()) {
			fprintf(stderr, "Server: warning: received packet from non-existing player-id %u. Ignoring.\n", client_id);
			return;
		}
	}

	unsigned int seq;
	socket.copy_from_packet_buffer(&seq, PACKET_SEQ_NUMBER_LIMITS);

	command_arg_mask_union cmdbyte_arg_mask;
	socket.copy_from_packet_buffer(&cmdbyte_arg_mask.us, CMD_ARG_MASK_LIMITS);

	if (cmdbyte_arg_mask.ch[0] == C_PONG) {
		id_client_map::iterator client_iter = get_client_by_id(client_id);
		if (client_iter != clients.end()) {
			struct Client &c = client_iter->second;
			unsigned int embedded_pong_seq_number;
			socket.copy_from_packet_buffer(&embedded_pong_seq_number, 12, 16);
			if (embedded_pong_seq_number == c.active_ping_seq_number) {
				fprintf(stderr, "Received C_PONG from client %d with seq_number %d (took %d us)\n", c.id, embedded_pong_seq_number, (int)c.ping_timer.get_us());
				c.active_ping_seq_number = 0;
			}
			else {
				fprintf(stderr, "Received C_PONG from client %d, but the seq_number embedded in the datagram (%d, bytes 12-16) doesn't match expected seq_number (%d)!\n", embedded_pong_seq_number, c.active_ping_seq_number);
			}
		}
		else {
			fprintf(stderr, "Server: error: couldn't find client with id %d\n", client_id);
		}
	}

	else if (cmdbyte_arg_mask.ch[0] == C_HANDSHAKE) {
		std::string name_string(socket.get_inbound_buffer() + 12);
		unsigned short new_id = add_client(from, name_string);
		id_client_map::iterator newcl = get_client_by_id(new_id);
		if(newcl != clients.end()) {
			handshake(&newcl->second);
		}
		else {
			fprintf(stderr, "server: internal error @ handshake (this shouldn't be happening!).\n");
		}
		post_peer_list();
	}
	else if (cmdbyte_arg_mask.ch[0] == C_QUIT) {
		remove_client(clients.find(client_id));
		fprintf(stderr, "Received C_QUIT from client %d\n", client_id);
	}
	else {
		fprintf(stderr, "received unknown cmdbyte %u with arg %u\n", cmdbyte_arg_mask.ch[0], cmdbyte_arg_mask.ch[1]);
	}

}

unsigned short Server::add_client(struct sockaddr_in *newclient_saddr, const std::string &name) {
	++num_clients;
	
	struct Client newclient;
	newclient.address = *newclient_saddr;
	newclient.id = num_clients;
	newclient.name = name;
	
	newclient.ip_string = get_dot_notation_ipv4(newclient_saddr);
	
	clients.insert(std::pair<unsigned short, struct Client>(newclient.id, newclient));

	fprintf(stderr, "Server: added client \"%s\" with id %d\n", newclient.name.c_str(), newclient.id);

	return newclient.id;
}

id_client_map::iterator Server::remove_client(id_client_map::iterator &iter) {
	id_client_map::iterator it;
	if (iter != clients.end()) {
		it = clients.erase(iter);
		return it;
	}
	else {
		fprintf(stderr, "Server: remove client: internal error: couldn't find client %u from the client_id map (this shouldn't be happening)!\n");
	}
}

std::unordered_map<unsigned short, struct Client>::iterator Server::get_client_by_id(unsigned short id) {
	return clients.find(id);
}

void Server::handshake(struct Client *client) {
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = S_HANDSHAKE_OK;
	
	protocol_make_header(socket.get_outbound_buffer(), client->id, client->seq_number, cmd_arg_mask.us);
	socket.copy_to_outbound_buffer(&client->id, sizeof(client->id), 12);
	int bytes = send_data_to_client(*client, 14);
}

int Server::send_data_to_client(struct Client &client, size_t data_size) {
	// use this wrapper in order to appropriately increment seq numberz
	int bytes = socket.send_data(&client.address, data_size);
	++client.seq_number;
	return bytes;
}

void Server::ping_clients() {
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = S_PING;
	id_client_map::iterator iter = clients.begin();
	while (iter != clients.end()) {
		struct Client &c = iter->second;
		if (c.active_ping_seq_number == 0) { // no ping request active, make a new one :P
			fprintf(stderr, "Sending S_PING to client %d. (seq = %d)\n", c.id, c.seq_number);
			c.active_ping_seq_number = c.seq_number;
			protocol_make_header(socket.get_outbound_buffer(), ID_SERVER, c.seq_number, cmd_arg_mask.us);
			send_data_to_client(c, 12);
			c.ping_timer.init();
			c.ping_timer.begin();
			++iter;
		}
		else if (c.ping_timer.get_s() > 5) {
			fprintf(stderr, "client %u: unanswered S_PING requests for more than 5 seconds, kicking.\n", c.id);
			iter = remove_client(iter);
		}
		else {
			++iter;
		}
	}

}

void Server::post_peer_list() {
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = S_PEER_LIST;
	cmd_arg_mask.ch[1] = (unsigned char) clients.size();
	protocol_make_header(socket.get_outbound_buffer(), ID_SERVER, 0, cmd_arg_mask.us);

	char peer_buf[PACKET_SIZE_MAX-12];
	size_t offset = 0;

	id_client_map::iterator iter = clients.begin();
	while (iter != clients.end()) {
		struct Client &c = iter->second;
		// check if offset > something
		offset += sprintf_s(peer_buf + offset, PACKET_SIZE_MAX-12, "%u/%s/%s|", c.id, c.name.c_str(), c.ip_string.c_str());
		++iter;
	}
	peer_buf[offset] = '\0';
	socket.copy_to_outbound_buffer(peer_buf, offset, 12);
	
	iter = clients.begin();
	
	fprintf(stderr, "Server: sending peer list to all clients.\n");
	
	while (iter != clients.end()) {
		protocol_update_seq_number(socket.get_outbound_buffer(), iter->second.seq_number);

		send_data_to_client(iter->second, offset+12);
		++iter;
	}
}

void Server::broadcast_shutdown_message() {
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = S_SHUTDOWN;
	cmd_arg_mask.ch[1] = 0x00;
	protocol_make_header(socket.get_outbound_buffer(), ID_SERVER, 0, cmd_arg_mask.us);

	id_client_map::iterator iter = clients.begin();
	while (iter != clients.end()) {
		struct Client &c = iter->second;
		protocol_update_seq_number(socket.get_outbound_buffer(), c.seq_number);
		send_data_to_client(c, 12);
		fprintf(stderr, "Sending S_SHUTDOWN to remote %s\n", c.ip_string);
		++iter;
	}
}



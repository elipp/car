#include "net/client.h"
#include "net/protocol.h"
#include "lin_alg.h"


const vec4 colors[8] = {
	vec4(1.0, 0.2, 0.2, 1.0),
	vec4(0.4, 0.7, 0.2, 1.0),
	vec4(0.4, 0.2, 1.0, 1.0),
	vec4(1.0, 1.0, 0.0, 1.0),
	vec4(0.0, 1.0, 1.0, 1.0),
	vec4(1.0, 0.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(0.0, 0.8, 0.2, 1.0)
};

std::thread LocalClient::keystate_thread;
std::thread LocalClient::listen_thread;
int LocalClient::_connected = 0;
Socket LocalClient::socket;
struct Client LocalClient::client;
struct sockaddr_in LocalClient::remote_sockaddr;
std::unordered_map<unsigned short, struct Peer> LocalClient::peers;
bool LocalClient::_bad = false;
int LocalClient::_listening = 0;


int LocalClient::init(const std::string &name, const std::string &remote_ip, unsigned short int port) {

	client.info.name = name;
	client.info.ip_string = "127.0.0.1";
	client.info.id = ID_CLIENT_UNASSIGNED;	// not assigned
	
	client.seq_number = 0;

	remote_sockaddr.sin_family = AF_INET;
	remote_sockaddr.sin_port = htons(port);
	remote_sockaddr.sin_addr.S_un.S_addr = inet_addr(remote_ip.c_str());
	Socket::initialize();

	socket = Socket(50001, SOCK_DGRAM, true);

	if (socket.bad()) { 
		onScreenLog::print( "LocalClient: socket init failed.\n");
		return 0; 
	}

	if (!handshake()) { return 0; }

	listen_thread = std::thread(listen);	
	keystate_thread = std::thread(keystate_loop);

	return 1;
}

void LocalClient::keystate_loop() {

	#define KEYSTATE_GRANULARITY_MS 25
	static _timer keystate_timer;
	keystate_timer.begin();
	while (_listening) {
		if (keystate_timer.get_ms() > KEYSTATE_GRANULARITY_MS) {
			update_keystate(WM_KEYDOWN_KEYS);
			post_keystate();
			keystate_timer.begin();
			long wait = KEYSTATE_GRANULARITY_MS - keystate_timer.get_ms();
			if (wait > 1) { Sleep(wait); }
		}
	}

}

int LocalClient::handshake() {

	command_arg_mask_union command_arg_mask;

	command_arg_mask.ch[0] = C_HANDSHAKE;
	command_arg_mask.ch[1] = 0xFF;

	strcpy_s(socket.get_outbound_buffer() + PTCL_HEADER_LENGTH, PACKET_SIZE_MAX-PTCL_HEADER_LENGTH, client.info.name.c_str());
	protocol_make_header(socket.get_outbound_buffer(), client.info.id, client.seq_number, command_arg_mask.us);
		
	send_data_to_server(PTCL_HEADER_LENGTH + client.info.name.length());

#define RETRY_GRANULARITY_MS 1000
#define NUM_RETRIES 5
		
	onScreenLog::print( "LocalClient::sending handshake to remote.\n");
	
	int received_data = 0;
	for (int i = 0; i < NUM_RETRIES; ++i) {

		if (socket.wait_for_incoming_data(RETRY_GRANULARITY_MS) > 0) { 
			received_data = 1;
			break;
		}
		onScreenLog::print("Re-sending handshake to remote...\n");
		send_data_to_server(PTCL_HEADER_LENGTH + client.info.name.length());
	}

	if (!received_data) {
		onScreenLog::print("Handshake timed out.\n");
		_listening = 0;
		return 0;
	}
	
	struct sockaddr_in from;
	int bytes = socket.receive_data(&from);	// now we know we actually have received data in the socket

	int protocol_id;
	socket.copy_from_inbound_buffer(&protocol_id, PTCL_ID_BYTERANGE);
	if (protocol_id != PROTOCOL_ID) {
		onScreenLog::print( "LocalClient: handshake: protocol id mismatch (received %d)\n", protocol_id);
		return 0;
	}
	
	socket.copy_from_inbound_buffer(&client.info.id, PTCL_DATAFIELD_BYTERANGE(sizeof(client.info.id)));
	client.info.name = socket.get_inbound_buffer() + PTCL_HEADER_LENGTH + sizeof(client.info.id);
	onScreenLog::print( "Client: received player id %d and name %s from %s. =)\n", client.info.id, client.info.name.c_str(), get_dot_notation_ipv4(&from).c_str());
	
	_listening = 1;

	return 1;

}

void LocalClient::pong(unsigned seq_number) {
	//onScreenLog::print("Received S_PING from remote (seq_number = %u)\n", seq_number);
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = C_PONG;
	cmd_arg_mask.ch[1] = 0x00;
	protocol_make_header(socket.get_outbound_buffer(), client.info.id, client.seq_number, cmd_arg_mask.us);
	socket.copy_to_outbound_buffer(&seq_number, sizeof(seq_number), PTCL_HEADER_LENGTH);
	send_data_to_server(PTCL_HEADER_LENGTH + sizeof(seq_number));
}

static inline std::vector<struct peer_info_t> process_peer_list_string(const char* buffer) {
	std::vector<struct peer_info_t> peers;
	std::vector<std::string> sub = split(buffer, '|');
	auto it = sub.begin();
	while (it != sub.end()) {
		std::vector<std::string> tokens = split(*it, '/');
		if (tokens.size() != 4) {
			//onScreenLog::print( "warning: process_peer_list_string: tokens.size() != 3 (%d): dump = \"%s\"\n", tokens.size(), it->c_str());
		}
		else {
			peers.push_back(struct peer_info_t(std::stoi(tokens[0]), tokens[1], tokens[2], std::stoi(tokens[3])));
		}
		++it;
	}
	return peers;	
}

void LocalClient::handle_current_packet() {
	int protocol_id;
	socket.copy_from_inbound_buffer(&protocol_id, PTCL_ID_BYTERANGE);
	if (protocol_id != PROTOCOL_ID) {
		onScreenLog::print( "dropping packet. Reason: protocol_id mismatch (%d)\n", protocol_id);
		return;
	}

	unsigned short sender_id;
	socket.copy_from_inbound_buffer(&sender_id, PTCL_SENDER_ID_BYTERANGE);
	if (sender_id != ID_SERVER) {
		//onScreenLog::print( "unexpected sender id. expected ID_SERVER (%x), got %x instead.\n", ID_SERVER, sender_id);
	}
	unsigned int seq_number;
	socket.copy_from_inbound_buffer(&seq_number, PTCL_SEQ_NUMBER_BYTERANGE);

	command_arg_mask_union cmd_arg_mask;
	socket.copy_from_inbound_buffer(&cmd_arg_mask.us, PTCL_CMD_ARG_BYTERANGE);

	const unsigned char cmd = cmd_arg_mask.ch[0];

	if (cmd == S_PING) {
		pong(seq_number);
	}
	else if (cmd == S_POSITION_UPDATE) {
		update_positions();
	}
	else if (cmd == S_SHUTDOWN) {
		onScreenLog::print( "Received S_SHUTDOWN from server.\n");
		// shut down.
	}
	else if (cmd == S_PEER_LIST) {
		construct_peer_list();
	}
	else if (cmd == S_CLIENT_DISCONNECT) {
		unsigned short id;
		socket.copy_from_inbound_buffer(&id, PTCL_DATAFIELD_BYTERANGE(sizeof(id)));
		auto it = peers.find(id);
		if (it == peers.end()) {
			//onScreenLog::print( "Warning: received S_CLIENT_DISCONNECT with unknown id %u.\n", id);
		}
		else {
			onScreenLog::print( "Client %s (id %u) disconnected.\n", it->second.info.name.c_str(), id);
			peers.erase(id);
		}
	}

}

void LocalClient::update_positions() {
	command_arg_mask_union cmd_arg_mask;
	socket.copy_from_inbound_buffer(&cmd_arg_mask, PTCL_CMD_ARG_BYTERANGE);
	unsigned num_clients = cmd_arg_mask.ch[1];


	for (unsigned i = 0; i < num_clients; ++i) {
		unsigned short id;
		socket.copy_from_inbound_buffer(&id, PTCL_HEADER_LENGTH + i*PTCL_POS_DATA_SIZE, PTCL_HEADER_LENGTH + i*PTCL_POS_DATA_SIZE + sizeof(id));
		auto it = peers.find(id);
		if (it == peers.end()) {
			//onScreenLog::print( "update_positions: unknown peer id included in peer list (%u)\n", id);
		}
		else {
			size_t offset = PTCL_HEADER_LENGTH + i*PTCL_POS_DATA_SIZE + sizeof(id);
			socket.copy_from_inbound_buffer(&it->second.car, offset, offset + sizeof(struct Car));
			//onScreenLog::print( "update_positions: copied car %u position data as \n", id);
			//it->second.car.position().print();
		}
	}
	
}

void LocalClient::listen() {
	
	struct sockaddr_in from;
	
	while (_listening) {
		int bytes = socket.receive_data(&from);
		if (bytes >= 0) {
			//std::string	ip_str = get_dot_notation_ipv4(&from);
			//onScreenLog::print( "Received %d bytes from %s\n", bytes, ip_str.c_str());
			handle_current_packet();
		}

	}
	onScreenLog::print( "LocalClient::stopping listen.\n");
}

void LocalClient::post_keystate() {
	static char keystate_buffer[PACKET_SIZE_MAX];
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = C_KEYSTATE;
	cmd_arg_mask.ch[1] = client.keystate;
	protocol_make_header(keystate_buffer, client.info.id, client.seq_number, cmd_arg_mask.us);
	send_data_to_server(keystate_buffer, PTCL_HEADER_LENGTH);
}

void LocalClient::post_quit_message() {
	//onScreenLog::print( "posting quit message\n");
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = C_QUIT;
	protocol_make_header(socket.get_outbound_buffer(), client.info.id, client.seq_number, cmd_arg_mask.us);
	int bytes = send_data_to_server(PTCL_HEADER_LENGTH);
}
void LocalClient::update_keystate(const bool *keys) {
	client.keystate = 0x0;

	if (keys[VK_UP]) { client.keystate |= C_KEYSTATE_UP; }
	if (keys[VK_DOWN]) { client.keystate |= C_KEYSTATE_DOWN; }
	if (keys[VK_LEFT]) { client.keystate |= C_KEYSTATE_LEFT; }
	if (keys[VK_RIGHT]) { client.keystate |= C_KEYSTATE_RIGHT; }
}

void LocalClient::quit() {
	
	_listening = 0;
	if (listen_thread.joinable()) { listen_thread.join(); }
	if (keystate_thread.joinable()) { keystate_thread.join(); }
	post_quit_message();
	socket.close();
	WSACleanup();
}

int LocalClient::send_data_to_server(size_t size) {
	int bytes = socket.send_data(&remote_sockaddr, size);
	++client.seq_number;
	return bytes;
}

int LocalClient::send_data_to_server(const char* buffer, size_t size) {

	int bytes = socket.send_data(&remote_sockaddr, buffer, size);
	++client.seq_number;
	return bytes;
}

void LocalClient::construct_peer_list() {
					
	std::vector<struct peer_info_t> peer_list = process_peer_list_string(socket.get_inbound_buffer() + PTCL_HEADER_LENGTH + 1);
		
	for (auto &it : peer_list) {
		//onScreenLog::print( "peer data: id = %u, name = %s, ip_string = %s\n", it.id, it.name.c_str(), it.ip_string.c_str());
		auto map_iter = peers.find(it.id);
		if (map_iter == peers.end()) {
			peers.insert(std::pair<unsigned short, struct Peer>(it.id, struct Peer(it.id, it.name, it.ip_string, it.color)));
			onScreenLog::print("Client %s (id %u) connected from %s.\n", it.name.c_str(), it.id, it.ip_string.c_str());
		}
		else {
			if (!(map_iter->second.info == it)) {
				onScreenLog::print( "warning: peer was found in the peer list, but peer_info_t discrepancies were discovered.\n");
			}
		}
	}


}

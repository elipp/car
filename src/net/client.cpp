#include "net/client.h"
#include "net/protocol.h"
#include "net/client_funcs.h"
#include "lin_alg.h"

#include <string>

static const vec4 colors[8] = {
	vec4(1.0, 0.2, 0.2, 1.0),
	vec4(0.4, 0.7, 0.2, 1.0),
	vec4(0.4, 0.2, 1.0, 1.0),
	vec4(1.0, 1.0, 0.0, 1.0),
	vec4(0.0, 1.0, 1.0, 1.0),
	vec4(1.0, 0.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(0.0, 0.8, 0.2, 1.0)
};

LocalClient::Listen LocalClient::Listener;
LocalClient::Keystate LocalClient::KeystateManager;

NetTaskThread LocalClient::parent(LocalClient::connect);

Socket LocalClient::socket;
struct Client LocalClient::client;
struct sockaddr_in LocalClient::remote_sockaddr;
std::unordered_map<unsigned short, struct Peer> LocalClient::peers;
int LocalClient::_connected = 0;
bool LocalClient::_shutdown_requested = false;

unsigned short LocalClient::port = 50001;

void LocalClient::connect() {
	
	int success = handshake();
	if (!success) { 
		_shutdown_requested = true;
		return; 
	}

	Listener.start();
	KeystateManager.start();

	while (parent.running() && _connected) {
		// kind of stupid though :P A sleeping parent thread for the client net code
		Sleep(500);
	}

exit:
	KeystateManager.stop();
	Listener.stop();

	peers.clear();
	_connected = 0;
}

int LocalClient::start(const std::string &ip_port_string) {
	
	client.info.ip_string = "127.0.0.1";
	client.info.id = ID_CLIENT_UNASSIGNED;	// not assigned
	
	std::vector<std::string> tokens = split(ip_port_string, ':');

	if (tokens.size() != 2) {
		onScreenLog::print("connect: Error: badly formatted ip_port_string (%s)!\n", ip_port_string.c_str());
		return 0;
	}

	std::string remote_ip = tokens[0];
	unsigned short remote_port = std::stoi(tokens[1]);

	client.seq_number = 0;

	remote_sockaddr.sin_family = AF_INET;
	remote_sockaddr.sin_port = htons(remote_port);
	remote_sockaddr.sin_addr.S_un.S_addr = inet_addr(remote_ip.c_str());
	Socket::initialize();

	socket = Socket(port, SOCK_DGRAM, true);
	
	client.address = socket.get_own_addr();	// :D

	if (socket.bad()) { 
		onScreenLog::print( "LocalClient: socket init failed.\n");
		_shutdown_requested = true;
		return 0;
	}
	onScreenLog::print("client: attempting to connect to %s:%u.\n", remote_ip.c_str(), remote_port);

	parent.start();	// calls connect with the above-specified settings
	return 1;
}

void LocalClient::stop() {
	static const PTCLHEADERDATA TERMINATE_PACKET = { PROTOCOL_ID, 0, ID_SERVER, C_TERMINATE };
	// the packet of death. this sets _connected = false and breaks out of the listen loop gracefully (ie. without calling WSACleanup) :D
	socket.send_data(&socket.get_own_addr(), (const char*)&TERMINATE_PACKET, sizeof(TERMINATE_PACKET));
	parent.stop();
	socket.close();
	_connected = 0;
	_shutdown_requested = false;
}

void LocalClient::keystate_task() {
	KeystateManager.keystate_loop();
}

void LocalClient::Keystate::keystate_loop() {
	
	#define KEYSTATE_GRANULARITY_MS 25	// this could be overly tight though.
	static _timer keystate_timer;
	keystate_timer.begin();
	while (thread.running() && _connected) {
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
	
	PTCLHEADERDATA HANDSHAKE_HEADER
		= protocol_make_header(client.seq_number, ID_CLIENT_UNASSIGNED, C_HANDSHAKE);

	protocol_copy_header(parent.buffer, &HANDSHAKE_HEADER);
	parent.copy_to_buffer(client.info.name.c_str(), client.info.name.length(), PTCL_HEADER_LENGTH);

	send_data_to_server(parent.buffer, PTCL_HEADER_LENGTH + client.info.name.length());

			
#define RETRY_GRANULARITY_MS 1000
#define NUM_RETRIES 5
		
	onScreenLog::print( "LocalClient::sending handshake to remote.\n");
	
	// FIXME: currently broken when handshaking with localhost :P
	// the WS2 select() call reports "data available" when sending to 127.0.0.1, despite different port.

	int received_data = 0;
	for (int i = 0; i < NUM_RETRIES; ++i) {
		int select_r = socket.wait_for_incoming_data(RETRY_GRANULARITY_MS);
		if (select_r > 0) { 
			// could be wrong data. should check the data we received and retry if it wasn't the correct stuff
			received_data = 1;
			break;
		}
		onScreenLog::print("Re-sending handshake to remote...\n");
		send_data_to_server(parent.buffer, PTCL_HEADER_LENGTH + client.info.name.length());
	}

	if (!received_data) {
		onScreenLog::print("Handshake timed out.\n");
		_shutdown_requested = true;
		return 0;
	}
	
	struct sockaddr_in from;
	int bytes = socket.receive_data(parent.buffer, &from);

	PTCLHEADERDATA header;
	protocol_get_header_data(parent.buffer, &header);

	if (header.protocol_id != PROTOCOL_ID) {
		onScreenLog::print( "LocalClient: handshake: protocol id mismatch (received %d)\n", header.protocol_id);
		_shutdown_requested = true;
		return 0;
	}

	const unsigned char &cmd = header.cmd_arg_mask.ch[0];

	if (cmd != S_HANDSHAKE_OK) {
		onScreenLog::print("LocalClient: handshake: received cmdbyte != S_HANDSHAKE_OK (%x). Handshake failed.\n", cmd);
		_shutdown_requested = true;
		return 0;
	}

	// get player id embedded within the packet
	parent.copy_from_buffer(&client.info.id, sizeof(client.info.id), PTCL_HEADER_LENGTH);

	// get player name received from server
	client.info.name = std::string(parent.buffer + PTCL_HEADER_LENGTH + sizeof(client.info.id));
	onScreenLog::print( "Client: received player id %d and name %s from %s. =)\n", client.info.id, client.info.name.c_str(), get_dot_notation_ipv4(&from).c_str());
	
	_connected = true;
	return 1;

}

void LocalClient::Listen::pong(unsigned seq_number) {
	//onScreenLog::print("Received S_PING from remote (seq_number = %u)\n", seq_number);
	PTCLHEADERDATA PONG_HEADER = { PROTOCOL_ID, client.seq_number, client.info.id, C_PONG };
	thread.copy_to_buffer(&PONG_HEADER, sizeof(PONG_HEADER), 0);
	thread.copy_to_buffer(&seq_number, sizeof(seq_number), PTCL_HEADER_LENGTH);
	send_data_to_server(thread.buffer, PTCL_HEADER_LENGTH + sizeof(seq_number));
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


void LocalClient::send_chat_message(const std::string &msg) {
	if (!_connected) { 
		onScreenLog::print("chat: not connected to server.\n");
		return; 
	}
	static char chat_msg_buffer[PACKET_SIZE_MAX];
	
	uint8_t message_len = min(msg.length(), PACKET_SIZE_MAX - PTCL_HEADER_LENGTH - 1);

	PTCLHEADERDATA CHAT_HEADER 
		= protocol_make_header( client.seq_number, client.info.id, C_CHAT_MESSAGE, message_len);
	
	protocol_copy_header(chat_msg_buffer, &CHAT_HEADER);
	memcpy(chat_msg_buffer + PTCL_HEADER_LENGTH, msg.c_str(), message_len);
	size_t total_size = PTCL_HEADER_LENGTH + message_len;
	send_data_to_server(chat_msg_buffer, total_size);
}

void LocalClient::Listen::handle_current_packet() {

	PTCLHEADERDATA header;
	protocol_get_header_data(thread.buffer, &header);

	if (header.protocol_id != PROTOCOL_ID) {
		onScreenLog::print( "dropping packet. Reason: protocol_id mismatch (%d)\n", header.protocol_id);
		return;
	}

	if (header.sender_id != ID_SERVER) {
		//onScreenLog::print( "unexpected sender id. expected ID_SERVER (%x), got %x instead.\n", ID_SERVER, sender_id);
		return;
	}

	const unsigned char &cmd = header.cmd_arg_mask.ch[0];
	if (cmd == S_POSITION_UPDATE) {
		update_positions();
	}
	else if (cmd == S_PING) {
		pong(header.seq_number);
	}
	else if (cmd == S_CLIENT_CHAT_MESSAGE) {
		// the sender id is embedded into the datafield (bytes 12-14) of the packet
		unsigned short sender_id = 0;
		thread.copy_from_buffer(&sender_id, sizeof(sender_id), PTCL_HEADER_LENGTH);
		auto it = peers.find(sender_id);
		if (it != peers.end()) {
			std::string chatmsg(thread.buffer + PTCL_HEADER_LENGTH + sizeof(sender_id));
			onScreenLog::print("<%s> %s: %s\n", get_timestamp().c_str(), it->second.info.name.c_str(), chatmsg.c_str());
		}
		else {
			// perhaps request a new peer list from the server. :P
			onScreenLog::print("warning: server broadcast S_CLIENT_CHAT_MESSAGE with unknown sender id %u!\n", sender_id);
		}
	}
	else if (cmd == S_SHUTDOWN) {
		onScreenLog::print( "Received S_SHUTDOWN from server (server going down).\n");
		_connected = 0; // this will break from the recvfrom loop gracefully
		_shutdown_requested = true;	// this will cause LocalClient::stop() to be called from the main (rendering) thread
		//LocalClient::stop();
	}
	else if (cmd == C_TERMINATE) {
		fprintf(stderr, "Received C_TERMINATE from self. Stopping.\n");
		onScreenLog::print("Received C_TERMINATE from self. Stopping.\n");
		_connected = 0;
		_shutdown_requested = true;
	}
	else if (cmd == S_PEER_LIST) {
		construct_peer_list();
	}
	else if (cmd == S_CLIENT_DISCONNECT) {
		unsigned short id;
		thread.copy_from_buffer(&id, sizeof(id), PTCL_HEADER_LENGTH);
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

void LocalClient::Listen::update_positions() {
	command_arg_mask_union cmd_arg_mask;
	thread.copy_from_buffer(&cmd_arg_mask, PTCL_CMD_ARG_FIELD);
	unsigned num_clients = cmd_arg_mask.ch[1];

	static const size_t serial_data_size = sizeof(Car().data_serial);
	static const size_t PTCL_POS_DATA_SIZE = sizeof(unsigned short) + serial_data_size;

	size_t total_size = PTCL_HEADER_LENGTH;

	for (unsigned i = 0; i < num_clients; ++i) {
		unsigned short id;
		thread.copy_from_buffer(&id, sizeof(id), PTCL_HEADER_LENGTH + i*PTCL_POS_DATA_SIZE);
		auto it = peers.find(id);
		if (it == peers.end()) {
			//onScreenLog::print( "update_positions: unknown peer id included in peer list (%u)\n", id);
		}
		else {
			size_t offset = PTCL_HEADER_LENGTH + i*PTCL_POS_DATA_SIZE + sizeof(id);
			thread.copy_from_buffer(&it->second.car.data_serial, serial_data_size, offset);
			//onScreenLog::print( "update_positions: copied car %u position data as \n", id);
			//it->second.car.position().print();
		}
	}
	
}

void LocalClient::listen_task() {
	Listener.listen();
}

void LocalClient::Listen::listen() {
	
	struct sockaddr_in from;
	
	while (thread.running() && _connected) {
		int bytes = socket.receive_data(thread.buffer, &from);
		if (bytes >= 0) { handle_current_packet(); }
	}

	post_quit_message();

}

void LocalClient::Keystate::post_keystate() {
	PTCLHEADERDATA KEYSTATE_HEADER = { PROTOCOL_ID, client.seq_number, client.info.id, C_KEYSTATE };
	KEYSTATE_HEADER.cmd_arg_mask.ch[1] = client.keystate;
	thread.copy_to_buffer(&KEYSTATE_HEADER, sizeof(KEYSTATE_HEADER), 0);
	send_data_to_server(thread.buffer, PTCL_HEADER_LENGTH);
}

void LocalClient::Keystate::update_keystate(const bool *keys) {
	client.keystate = 0x0;

	if (keys[VK_UP]) { client.keystate |= C_KEYSTATE_UP; }
	if (keys[VK_DOWN]) { client.keystate |= C_KEYSTATE_DOWN; }
	if (keys[VK_LEFT]) { client.keystate |= C_KEYSTATE_LEFT; }
	if (keys[VK_RIGHT]) { client.keystate |= C_KEYSTATE_RIGHT; }
}

void LocalClient::Listen::post_quit_message() {
	PTCLHEADERDATA QUIT_HEADER 
		= protocol_make_header( client.seq_number, client.info.id, C_QUIT);
	protocol_copy_header(thread.buffer, &QUIT_HEADER);
	send_data_to_server(thread.buffer, PTCL_HEADER_LENGTH);
}

void LocalClient::parse_user_input(const std::string s) {
	if (s.length() <= 0) { return; }
	if (s[0] != '/') {
		// - we're dealing with a chat message
		send_chat_message(s);
	}
	else {
		std::vector<std::string> input = split(s, ' ');
		if (input.size() <= 0) { return; }
		auto it = funcs.find(input[0]);
		if (it != funcs.end()) { 
			it->second(input);
		}
		else {
			onScreenLog::print("%s: command not found. See /help for a list of available commands.\n", input[0].c_str());
		}
		
	}
}

void LocalClient::quit() {
	LocalClient::stop();
	Socket::deinitialize();
}

int LocalClient::send_data_to_server(const char* buffer, size_t size) {

	int bytes = socket.send_data(&remote_sockaddr, buffer, size);
	++client.seq_number;
	return bytes;
}
void LocalClient::set_name(const std::string &nick) {
	if (!_connected) {
		client.info.name = nick;
		onScreenLog::print("Name set to %s.\n", nick.c_str());
	} else {
		onScreenLog::print("set_name: cannot change name while connected to server.\n");
	}
}

void LocalClient::Listen::construct_peer_list() {
					
	std::vector<struct peer_info_t> peer_list = process_peer_list_string(thread.buffer + PTCL_HEADER_LENGTH + 1);
		
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

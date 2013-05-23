#include "net/net_client.h"
#include "net/net_protocol.h"

std::thread LocalClient::client_thread;
Socket LocalClient::socket;
struct Client LocalClient::client;
struct sockaddr_in LocalClient::remote_sockaddr;
std::unordered_map<unsigned short, struct Client> LocalClient::peers;
bool LocalClient::_bad = false;
int LocalClient::_listening = 0;


void LocalClient::start_thread() {
	if (!handshake()) {
		_bad = true;
		return;
	}
	listen();

}

int LocalClient::init(const std::string &name, const std::string &remote_ip, unsigned short int port) {
	memset(&client, 0, sizeof(client));

	client.name = name;
	client.ip_string = "127.0.0.1";
	
	client.seq_number = 0;
	client.id = 0xFF;	// not assigned

	remote_sockaddr.sin_family = AF_INET;
	remote_sockaddr.sin_port = htons(port);
	remote_sockaddr.sin_addr.S_un.S_addr = inet_addr(remote_ip.c_str());
	Socket::initialize();

	socket = Socket(50001, SOCK_DGRAM, false);

	if (socket.bad()) { 
		fprintf(stderr, "LocalClient: socket init failed.\n");
		return 0; 
	}

	client_thread = std::thread(start_thread);

	return 1;
}

int LocalClient::handshake() {

	command_arg_mask_union command_arg_mask;

	command_arg_mask.ch[0] = C_HANDSHAKE;
	command_arg_mask.ch[1] = 0xFF;

	strcpy_s(socket.get_outbound_buffer() + 12, PACKET_SIZE_MAX-12, client.name.c_str());
	protocol_make_header(socket.get_outbound_buffer(), client.id, client.seq_number, command_arg_mask.us);
	
	fprintf(stderr, "LocalClient::sending handshake to remote...\n");
	send_current_data(12 + client.name.length());

	struct sockaddr_in from;

	fprintf(stderr, "Awaiting for reply...\n");
	
#define TIMEOUT_MS 5000
#define RETRY_GRANULARITY_MS 250

	float milliseconds_accumulator = 0;
	int bytes = socket.receive_data(&from);

	while(bytes <= 0 && _listening == 1) {
		bytes = socket.receive_data(&from);
		if (milliseconds_accumulator > TIMEOUT_MS) {
			fprintf(stderr, "Handshake timed out (%d seconds)!\n", TIMEOUT_MS/1000.0);
			return 0;
		}
		Sleep(RETRY_GRANULARITY_MS);
		milliseconds_accumulator += RETRY_GRANULARITY_MS;
	}
	
	int protocol_id;
	socket.copy_from_packet_buffer(&protocol_id, PROTOCOL_ID_LIMITS);
	if (protocol_id != PROTOCOL_ID) {
		fprintf(stderr, "LocalClient: handshake: protocol id mismatch (received %d)\n", protocol_id);
		return 0;
	}
	
	socket.copy_from_packet_buffer(&client.id, 12, 14);
	fprintf(stderr, "Client: received player id %d from %s!\n", client.id, get_dot_notation_ipv4(&from).c_str());

	return 1;

}

void LocalClient::pong(unsigned seq_number) {
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = C_PONG;
	cmd_arg_mask.ch[1] = 0x00;
	protocol_make_header(socket.get_outbound_buffer(), client.id, client.seq_number, cmd_arg_mask.us);
	socket.copy_to_outbound_buffer(&seq_number, sizeof(seq_number), 12);
	send_current_data(12 + sizeof(seq_number));
}

void LocalClient::handle_current_packet() {
	int protocol_id;
	socket.copy_from_packet_buffer(&protocol_id, PROTOCOL_ID_LIMITS);
	if (protocol_id != PROTOCOL_ID) {
		fprintf(stderr, "dropping packet. Reason: protocol_id mismatch (%d)\n", protocol_id);
		return;
	}

	unsigned short sender_id;
	socket.copy_from_packet_buffer(&sender_id, SENDER_ID_LIMITS);
	if (sender_id != ID_SERVER) {
		fprintf(stderr, "unexpected sender id. expected ID_SERVER (%x), got %x instead.\n", ID_SERVER, sender_id);
	}
	unsigned int seq_number;
	socket.copy_from_packet_buffer(&seq_number, PACKET_SEQ_NUMBER_LIMITS);

	command_arg_mask_union cmd_arg_mask;

	socket.copy_from_packet_buffer(&cmd_arg_mask.us, CMD_ARG_MASK_LIMITS);

	if (cmd_arg_mask.ch[0] == S_PING) {
		pong(seq_number);
	}
	else if (cmd_arg_mask.ch[0] == S_SHUTDOWN) {
		fprintf(stderr, "Received S_SHUTDOWN from server.\n");
	}
	else if (cmd_arg_mask.ch[0] == S_PEER_LIST) {
		fprintf(stderr, "Received peer list. Raw dump: \n");
		buffer_print_raw(socket.get_inbound_buffer(), socket.current_data_length_in());
	}

}

void LocalClient::listen() {
	_listening = 1;
	struct sockaddr_in from;

	while (_listening) {
		int bytes = socket.receive_data(&from);
		if (bytes >= 0) {
			std::string	ip_str = get_dot_notation_ipv4(&from);
			fprintf(stderr, "Received %d bytes from %s\n", bytes, ip_str.c_str());
			handle_current_packet();
		}
	}
}

void LocalClient::post_quit_message() {
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = C_QUIT;
	protocol_make_header(socket.get_outbound_buffer(), client.id, client.seq_number, cmd_arg_mask.us);
	int bytes = send_current_data(12);
}

void LocalClient::quit() {
	fprintf(stderr, "Received S_SHUTDOWN from server. Stopping listen.\n");
	post_quit_message();	// a couple of times to make it more reliab;e
	post_quit_message();
	post_quit_message();
	socket.close();
	WSACleanup();
	_listening = 0;
	if(client_thread.joinable()) { client_thread.join(); }
}

int LocalClient::send_current_data(size_t size) {
	int bytes = socket.send_data(&remote_sockaddr, size);
	++client.seq_number;
	return bytes;
}

void LocalClient::construct_peer_list() {
	// if this was called, it means 
	std::string peer_list(socket.get_inbound_buffer() + 12);

}

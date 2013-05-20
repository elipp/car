#include "net/net_client.h"
#include "net/net_protocol.h"

std::thread LocalClient::client_thread;
Socket LocalClient::socket;
struct Client LocalClient::client;
struct sockaddr_in LocalClient::remote_sockaddr;
std::unordered_map<unsigned short, struct Client> LocalClient::peers;


int LocalClient::init(const std::string &name, const std::string &remote_ip, unsigned short int port) {
	memset(&client, 0, sizeof(client));

	client.name = _strdup(name.c_str());
	client.ip_string = _strdup("127.0.0.1");
	
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
	handshake();
}

int LocalClient::handshake() {

	command_arg_mask_union command_arg_mask;

	command_arg_mask.ch[0] = C_HANDSHAKE;
	command_arg_mask.ch[1] = 0xFF;

	int name_len = strlen(client.name);
	strcpy_s(socket.get_outbound_buffer() + 12, PACKET_SIZE_MAX-12, client.name);
	protocol_make_header(socket.get_outbound_buffer(), &client, command_arg_mask.us);
	
	fprintf(stderr, "LocalClient::sending handshake to remote...\n");
	socket.send_data(&remote_sockaddr, 12 + name_len);

	struct sockaddr_in from;
	int bytes = 0;
	fprintf(stderr, "Awaiting for reply...\n");
	while(bytes <= 0) {
		bytes = socket.receive_data(&from);
	}
	
	int protocol_id;
	socket.copy_from_packet_buffer(&protocol_id, 0, 4);
	if (protocol_id != PROTOCOL_ID) {
		fprintf(stderr, "LocalClient: protocol id mismatch (received %d)\n", protocol_id);

	}
	
	socket.copy_from_packet_buffer(&client.id, 12, 14);
	fprintf(stderr, "Client: received player id %d from %s!\n", client.id, get_dot_notation_ipv4(&from).c_str());

	return 1;

}

void LocalClient::handle_current_packet() {
	int protocol_id;
	socket.copy_from_packet_buffer(&protocol_id, 0, 4);
	if (protocol_id != PROTOCOL_ID) {
		fprintf(stderr, "dropping packet. Reason: protocol_id mismatch (%d)\n", protocol_id);
	}
	unsigned short sender_id;
	socket.copy_from_packet_buffer(&sender_id, 4, 6);
	if (sender_id != ID_SERVER) {
		fprintf(stderr, "unexpected sender id. expected ID_SERVER (%x), got %x instead.\n", ID_SERVER, sender_id);
	}
	command_arg_mask_union cmd_arg_mask;

	socket.copy_from_packet_buffer(&cmd_arg_mask.us, 6, 8);

}

static int _listening = 0;

void LocalClient::listen() {
	_listening = 1;
	struct sockaddr_in from;
	while (_listening) {
		int bytes = socket.receive_data(&from);
		if (bytes >= 0) {
			std::string	ip_str = get_dot_notation_ipv4(&from);
			fprintf(stderr, "Received %d bytes from %s.\n", bytes, ip_str.c_str());
			handle_current_packet();
		}
	}
}

void LocalClient::post_quit_message() {
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = C_QUIT;
	protocol_make_header(socket.get_outbound_buffer(), &client, cmd_arg_mask.us);
	socket.send_data(&remote_sockaddr, 12);
}

int LocalClient::send_current_data(size_t size) {
	int bytes = socket.send_data(&remote_sockaddr, size);
	++client.seq_number;
	return bytes;
}

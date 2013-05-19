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
	unsigned short command_arg_mask = 0;
	command_arg_mask |= C_HANDSHAKE & 0x00FF;
	command_arg_mask |= 0xFF00;
	int name_len = strlen(client.name);
	strcpy_s(socket.get_outbound_buffer() + 12, PACKET_SIZE_MAX-12, client.name);
	protocol_make_header(socket.get_outbound_buffer(), &client, command_arg_mask);
	
	socket.send_data(&remote_sockaddr, 12 + name_len);

	struct sockaddr_in from;
	int bytes = 0;
	while(bytes <= 0) {
		bytes = socket.receive_data(&from);
	}
	
	int protocol_id;
	socket.copy_from_packet_buffer(&protocol_id, 0, 4);
	if (protocol_id != PROTOCOL_ID) {
		fprintf(stderr, "LocalClient: protocol id mismatch (received %d)\n", protocol_id);

	}
	
	socket.copy_from_packet_buffer(&client.id, 12, 14);
	fprintf(stderr, "Client: received player id %d from %s\n", client.id, get_dot_notation_ipv4(&from).c_str());

	return 1;
	// the socket is blocking, so
}

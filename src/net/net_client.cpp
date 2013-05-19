#include "net/net_client.h"

Socket LocalClient::socket;
struct Client LocalClient::client;
struct sockaddr_in LocalClient::remote_sockaddr;
std::unordered_map<unsigned short, struct Client> LocalClient::peers;


int LocalClient::init(const std::string &name, const std::string &remote_ip, unsigned short int port) {
	memset(&client, 0, sizeof(client));

	client.name = _strdup(name.c_str());
	client.ip_string = _strdup("127.0.0.1");
	
	client.seq_number = 0;

	remote_sockaddr.sin_family = AF_INET;
	remote_sockaddr.sin_port = htons(port);
	remote_sockaddr.sin_addr.S_un.S_addr = inet_addr(remote_ip.c_str());

	if(!Socket::initialized()) { Socket::initialize(); }

	socket = Socket(port, IPPROTO_UDP, false);

	if (socket.bad()) { 
		logWindowOutput("LocalClient: socket init failed.\n");
		return 0; 
	}


}


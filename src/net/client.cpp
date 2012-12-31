#include "client.h"

static id_client_map peers;

static unsigned char packet_data[256];
static size_t maximum_packet_size = sizeof(packet_data);

static int sockfd, new_fd;
struct sockaddr_in si_local, si_other; 
static struct client local_client;

extern std::stringstream sstream;

int client_createUDPSocket(std::string ipstring, unsigned int port) {

	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd <= 0) {
		std::cerr << "client: Failed to create socket.\n";
		return -1;
	}

	const int default_port = 63355;
	unsigned short local_port = default_port;

	memset(&si_local, 0, sizeof(struct sockaddr_in));
	memset(&si_other, 0, sizeof(struct sockaddr_in));

	si_local.sin_family = AF_INET;
	si_local.sin_port = htons((unsigned short) local_port);
	si_local.sin_addr.s_addr = INADDR_ANY;

	si_other.sin_family = AF_INET;
	si_other.sin_port = htons((unsigned short) port);

	if (inet_aton(ipstring.c_str(), &si_other.sin_addr)==0) {
		std::cerr << "client: inet_aton failed. Invalid remote ip?\n";
		return -1;
	}

	while (bind(sockfd, (const sockaddr*)&si_local, sizeof(struct sockaddr_in) ) < 0 ) {
		std::cerr << "client: bind on port " << local_port << " failed. Trying another one...\n";
		++local_port;
		si_local.sin_port = htons((unsigned short) local_port);
	}
	std::cerr << "client: bind: port " << local_port << ".\n";
	
	if (fcntl(sockfd, F_SETFL, O_NONBLOCK, 1) == -1) {
		printf( "client: failed to set non-blocking socket\n" );
		return -1;
	}

	// consider non-blocking for client sockets as well
	return 1;
}

struct client *client_get_peer_by_id(int id) {
	id_client_map::iterator it = peers.find(id);
	if (it != peers.end()) {
		return &(*it).second;
	}
	else {
		return NULL;
	}
}

int client_send_packet(unsigned char* input_data, size_t len, struct sockaddr* target) {
	memcpy(packet_data, input_data, len);
	packet_data[len] = '\0';
	int sent_bytes = sendto(sockfd, (const char*)packet_data, len,
				0, target, sizeof(struct sockaddr_in));
	
	return sent_bytes;
}

std::string get_local_hostname() {
	#define HOSTNAME_MAX 64 
	char hostname[HOSTNAME_MAX];
	memset(hostname, 0, HOSTNAME_MAX);
	gethostname(hostname, HOSTNAME_MAX-1);
	std::string loginname = std::string(getlogin());
	return loginname+std::string("@")+std::string(hostname);		
}

int client_handshake(std::string ipstring) {

	local_client.name = get_local_hostname();
	local_client.address = si_local;
	local_client.port = ntohs(si_local.sin_port);
	std::string port_str = int_to_string(local_client.port);
	local_client.ip_string = std::string(ipstring);

	std::cerr << "local_client.port = " << ntohs(si_local.sin_port) << "\n";
	
	std::string handshake = std::string("HANDSHAKE:") + local_client.ip_string + ":" + local_client.name + ":" + port_str;
	std::cerr << "Sending handshake \"" + handshake + "\" to remote.\n";
	client_send_packet((unsigned char*)handshake.c_str(), handshake.length(), (struct sockaddr*)&si_other);

	struct sockaddr_in from;
	socklen_t from_length = sizeof(from);
	// add timeout :D
	
	fd_set readfds, masterfds;
	FD_ZERO(&masterfds);
	FD_SET(sockfd, &masterfds);
	memcpy(&readfds, &masterfds, sizeof(fd_set));

	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	int received_bytes;

	if (select(sockfd + 1, &readfds, NULL, NULL, &timeout) < 0) {
		std::cerr << "select failed.\n";
		return 0;
	} 
	else {
		if (FD_ISSET(sockfd, &readfds))
		{
			received_bytes = client_get_data_from_remote();
			
			std::string response ((const char*)packet_data);
			std::cerr << "received " << received_bytes << " bytes: \"" << response << "\".\n";

			if (response.find("HANDSHAKE:OK:") == std::string::npos) {
				std::cerr << "Invalid response from server.\n";
				return 0;
			}

			local_client.id_string = response.substr(response.length()-1, 1);
			std::cerr << "received local client id: " << local_client.id_string << " from server.\n";
			local_client.id = string_to_int(local_client.id_string);

			return 1;
		}
		else {
			std::cerr << "Handshake timed out.\n";
			return 0;
		}
	}

}

int client_get_data_from_remote() {

	memset(packet_data, 0, maximum_packet_size);

	static struct sockaddr_in from;
	static socklen_t from_length = sizeof(from);

	memset(&from, 0, sizeof(sockaddr_in));
	unsigned int received_bytes = recvfrom(sockfd, (char*)packet_data,
	maximum_packet_size, 0, (struct sockaddr*)&from, &from_length);

	packet_data[maximum_packet_size-1] = '\0';

	if (received_bytes > 0) { client_process_data_from_remote(); }

	return received_bytes;
}

int client_construct_peer_list(std::string peer_str) {

	std::vector<std::string> tokens = tokenize(peer_str, ':', 3);
	unsigned num_peers = string_to_int(tokens[2]);

	std::vector<std::string> peert = tokenize(peer_str, ';');	
	std::vector<std::string>::iterator iter = peert.begin() + 1;	// the first token contains SERVER:PEERS:# etc

	// peer_list_str = peer_list_str + c.id_string + ":" + c.name + ":" + c.ip_string;
	std::cerr << "client list: \n";
	while (iter != peert.end()) {
		std::cerr << (*iter) << "\n";
		struct client newclient;
		std::vector<std::string> subtokens = tokenize((*iter), ':');
		newclient.id_string = subtokens[0];
		newclient.id = string_to_int(subtokens[0]);
		newclient.name = subtokens[1];
		newclient.ip_string = subtokens[2];
		if (peers.find(newclient.id) == peers.end()) {
			std::cerr << "adding client " << newclient.name << " to map.\n";
			peers.insert(id_client_pair(newclient.id, newclient));
		}
		++iter;
	}
	if (num_peers != peers.size()) {
		std::cerr << "warning! construct_peer_list: num_peers != peers.size().\n";
		return -1;
	}
	return 1;
}

int client_process_data_from_remote() {
	//
	std::string packet_str = std::string((const char*)packet_data);
	// should probably check sender ip address before processing..
	if (packet_str.find("SERVER:QUIT") != std::string::npos) {
		std::cerr << "server going down. exiting.\n";
		return -1;
	}
	else if (packet_str.find("SERVER:PUPD:") != std::string::npos) {
		std::vector<std::string> t = tokenize(packet_str, ':', 3);
		int id = string_to_int(t[2]);
		client_update_position_data(id, packet_data);
		return 1;
	}
	else if (packet_str.find("SERVER:PEERS:") != std::string::npos) {
		client_construct_peer_list(packet_str);	
		return 1;
	}
	else {
		return 0;
	}
}

int client_post_quit_message() {
	std::string quit_msg = "QUIT:" + local_client.id_string;
	client_send_packet((unsigned char*)quit_msg.c_str(), quit_msg.length(), (struct sockaddr*)&si_other);
}

struct client client_get_local_client() { 
	return local_client;
}

int client_start(std::string ipstring, unsigned int port) {

	client_createUDPSocket(ipstring, port);
	int success = client_handshake(ipstring);
	if (!success) {
		return -1;
	}

	return 1;
}

int client_send_input_state_to_server(unsigned char keystate) {
	std::string state_string = std::string(("CLINPST:") + local_client.id_string + ";" + std::string((const char*)&keystate));
	client_send_packet((unsigned char*)state_string.c_str(), state_string.length(), (struct sockaddr*)&si_other);
}

int client_update_position_data(int player_id, unsigned char *data) {
	std::string tmp = std::string((const char*)data);
	size_t spos = tmp.find(';') + 1;
	struct client *c = client_get_peer_by_id(player_id);
	if (c) {
		struct car_serialized c_ser;
		memcpy((char*)&c_ser, data + spos, sizeof(struct car_serialized));
		c->lcar.update_from_serialized(c_ser);
	}
	else {
		std::cerr << "client_update_position_data: player_id " << player_id << " not found.\n";
	}
	return 1;
}
id_client_map& client_get_peers() {
	return peers;
}

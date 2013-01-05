#include "client.h"

static id_client_map peers;

static unsigned char packet_data[256];
static size_t maximum_packet_size = sizeof(packet_data);

static int sockfd, new_fd;
struct sockaddr_in si_local, si_other; 
static struct client local_client;
static _timer timer;

extern std::stringstream sstream;

int client_createUDPSocket(std::string ipstring, unsigned short int remote_port) {

	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd <= 0) {
		std::cerr << "client: Failed to create socket.\n";
		return -1;
	}

	const unsigned short int default_port = 63355;
	unsigned short local_port = default_port;

	memset(&si_local, 0, sizeof(struct sockaddr_in));
	memset(&si_other, 0, sizeof(struct sockaddr_in));

	si_local.sin_family = AF_INET;
	si_local.sin_port = htons((unsigned short) local_port);
	si_local.sin_addr.s_addr = INADDR_ANY;

	si_other.sin_family = AF_INET;
	si_other.sin_port = htons((unsigned short) remote_port);

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

struct client *client_get_peer_by_id(unsigned short id) {
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

int client_handshake() {	

	local_client.name = get_local_hostname();
	local_client.address = si_local;
	local_client.port = ntohs(si_local.sin_port);
	std::string port_str = int_to_string(local_client.port);
	local_client.ip_string = "127.0.0.1";

	size_t name_len = local_client.name.length();

	unsigned char handshake_buf[256];	// just use packet_data?
	handshake_buf[0] = C_HANDSHAKE;
	handshake_buf[1] = (unsigned char)((local_client.port & 0xFF00) >> 8);
	handshake_buf[2] = (unsigned char)(local_client.port & 0x00FF);
	handshake_buf[3] = (unsigned char)(local_client.name.length());
	memcpy(&handshake_buf[4], local_client.name.c_str(), name_len);
	size_t handshake_total_length = 4 + name_len;
	handshake_buf[handshake_total_length] = '\0';
	client_send_packet(handshake_buf, handshake_total_length, (struct sockaddr*)&si_other);

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
		return -1;
	} 
	else {
		if (FD_ISSET(sockfd, &readfds))
		{
			received_bytes = client_get_data_from_remote();
			//packet_data[received_bytes] = '\0';
			
			std::cerr << "handshake: received " << received_bytes << " bytes from remote.\n";

			if (packet_data[0] != S_HANDSHAKE_OK) {
				std::cerr << "Invalid response " << (int)packet_data[0] << " from server.\n";
				return -1;
			}
			local_client.id = packet_data[1];
			std::cerr << "local client id: " << local_client.id << ".\n";
//			peers.insert(id_client_pair(local_client.id, local_client));

			return 1;
		}
		else {
			std::cerr << "Handshake timed out.\n";
			return -1;
		}
	}

}

int client_get_data_from_remote() {

//	memset(packet_data, 0, maximum_packet_size);	***

	static struct sockaddr_in from;
	static socklen_t from_length = sizeof(from);

	memset(&from, 0, sizeof(sockaddr_in));
	unsigned int received_bytes = recvfrom(sockfd, (char*)packet_data,
	maximum_packet_size, 0, (struct sockaddr*)&from, &from_length);

	packet_data[maximum_packet_size-1] = '\0';

	if (received_bytes > 0) { return client_process_data_from_remote(); }

	return 1;
}

int client_construct_peer_list(std::string peer_str) {

	std::vector<std::string> tokens = tokenize(peer_str, ':', 3);
	unsigned short num_peers = peer_str[1];

	std::vector<std::string> peert = tokenize(peer_str, ';');	
	std::vector<std::string>::iterator iter = peert.begin() + 1;	// the first token contains SERVER:PEERS:# etc

	// peer_list_str = peer_list_str + c.id_string + ":" + c.name + ":" + c.ip_string;
	std::cerr << "client list: \n";
	while (iter != peert.end()) {
		std::cerr << (*iter) << "\n";
		struct client newclient;
		std::vector<std::string> subtokens = tokenize((*iter), ':');
		newclient.id = string_to_int(subtokens[0]);
		newclient.name = subtokens[1];
		newclient.ip_string = subtokens[2];
		if (peers.find(newclient.id) == peers.end()) {
			std::cerr << "adding client " << newclient.name << " with id " << newclient.id << " to map.\n";
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
	if (packet_data[0] == S_QUIT) {
		std::cerr << "server going down (S_QUIT). exiting.\n";
		return -1;
	}
	else if (packet_data[0] == S_PUPD) {
		client_update_position_data();
		return 1;
	}
	else if (packet_data[0] == S_PEER_LIST) {
		client_construct_peer_list(packet_str);	
		return 1;
	}
	else {
		return 0;
	}
}

int client_post_quit_message() {
	packet_data[0] = C_QUIT;
	packet_data[1] = local_client.id;
	packet_data[2] = 0;
	client_send_packet(packet_data, 2, (struct sockaddr*)&si_other);
}

struct client client_get_local_client() { 
	return local_client;
}

int client_start(std::string remote_ipstring, unsigned int remote_port) {

	client_createUDPSocket(remote_ipstring, remote_port);
	int success = client_handshake();
	if (!success) {
		return -1;
	}

	return 1;
}

int client_send_input_state_to_server(unsigned char keystate) {
	packet_data[0] = C_KEYSTATE;
	packet_data[1] = local_client.id;
	packet_data[2] = keystate;
	packet_data[3] = 0;
	client_send_packet(packet_data, 3, (struct sockaddr*)&si_other);
}

int client_update_position_data() {
	// assume data[0] == S_PUPD
	unsigned short player_id = packet_data[1];
	struct client *c = client_get_peer_by_id(player_id);
	if (c) {
		struct car_serialized c_ser;
		memcpy((char*)&c_ser, &packet_data[2], sizeof(struct car_serialized));
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

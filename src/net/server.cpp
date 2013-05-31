#include "net/server.h"

unsigned Server::seq_number = 0;

std::thread Server::listen_thread;
std::thread Server::ping_thread;
std::thread Server::position_thread;
std::unordered_map<unsigned short, struct Client> Server::clients;
unsigned Server::num_clients = 0;
Socket Server::socket;

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
			Server::handle_current_packet(&from);
		}
	}
	fprintf(stderr, "Stopping listen.\n");
}

int Server::init(unsigned short port) {
	Socket::initialize();
	socket = Socket(port, SOCK_DGRAM, true);	// for UDP and non-blocking
	if (socket.bad()) { fprintf(stderr, "Server::init, socket init error.\n"); return 0; }
	listen_thread = std::thread(listen);	// start listening thread
	position_thread = std::thread(calculate_state);
	ping_thread = std::thread(ping_clients);
	return 1;
}

void Server::shutdown() {
	// post quit messages to all clients
	listening = 0;
	fprintf(stderr, "Shutting down. Broadcasting S_SHUTDOWN.\n");
	broadcast_shutdown_message();
	fprintf(stderr, "Joining worker threads.\n");
	if (listen_thread.joinable()) { listen_thread.join(); }
	if (position_thread.joinable()) { position_thread.join(); }
	if (ping_thread.joinable()) { ping_thread.join(); } 
	fprintf(stderr, "Closing socket.\n");
	socket.close();
	fprintf(stderr, "Calling WSACleanup.\n");
	WSACleanup();
}


void Server::handle_current_packet(struct sockaddr_in *from) {
	int protocol_id;
	socket.copy_from_inbound_buffer(&protocol_id, PTCL_ID_BYTERANGE);
	if (protocol_id != PROTOCOL_ID) {
		fprintf(stderr, "dropping packet. Reason: protocol_id mismatch (%x)!\nDump:\n", protocol_id);
		//buffer_print_raw(socket.get_inbound_buffer(), socket.current_data_length_in());
		return;
	}

	unsigned short client_id;
	socket.copy_from_inbound_buffer(&client_id, PTCL_SENDER_ID_BYTERANGE);

	if (client_id != ID_CLIENT_UNASSIGNED) {
		id_client_map::iterator it = clients.find(client_id);
		if (it == clients.end()) {
			fprintf(stderr, "Server: warning: received packet from non-existing player-id %u. Ignoring.\n", client_id);
			return;
		}
	}

	unsigned int seq;
	socket.copy_from_inbound_buffer(&seq, PTCL_SEQ_NUMBER_BYTERANGE);

	command_arg_mask_union cmdbyte_arg_mask;
	socket.copy_from_inbound_buffer(&cmdbyte_arg_mask.us, PTCL_CMD_ARG_BYTERANGE);

	const unsigned char cmd = cmdbyte_arg_mask.ch[0];

	std::string ip = get_dot_notation_ipv4(from);
	//fprintf(stderr, "Received packet with size %u from %s (id = %u). Seq_number = %u\n", socket.current_data_length_in(), ip.c_str(), client_id, seq);
	
	auto client_iter = clients.find(client_id);
	
	if (cmd == C_KEYSTATE) {

		if (client_iter != clients.end()) {
			//if (seq > it->second.seq_number) {
			client_iter->second.keystate = cmdbyte_arg_mask.ch[1];
			//}
		}
		else {
			fprintf(stderr, "received keystate from unknown client %u, ignoring\n", client_iter->second.info.id);
		}
	}

	else if (cmd == C_PONG) {
		if (client_iter != clients.end()) {
			struct Client &c = client_iter->second;
			unsigned int embedded_pong_seq_number;
			socket.copy_from_inbound_buffer(&embedded_pong_seq_number, PTCL_DATAFIELD_BYTERANGE(sizeof(embedded_pong_seq_number)));
			if (embedded_pong_seq_number == c.active_ping_seq_number) {
				//fprintf(stderr, "Received C_PONG from client %d with seq_number %d (took %d us)\n", c.info.id, embedded_pong_seq_number, (int)c.ping_timer.get_us());
				c.active_ping_seq_number = 0;
			}
			else {
				fprintf(stderr, "Received C_PONG from client %d, but the seq_number embedded in the datagram (%d, bytes 12-16) doesn't match expected seq_number (%d)!\n", embedded_pong_seq_number, c.active_ping_seq_number);
			}
		}
		else {
			fprintf(stderr, "Server: error: couldn't find client with id %d (this shouldn't be happening!)\n", client_id);
		}
	}

	else if (cmd == C_HANDSHAKE) {
		std::string name_string(socket.get_inbound_buffer() + PTCL_HEADER_LENGTH);
		static unsigned rename = 0;
		int needs_renaming = 0;
		for (auto &name_iter : clients) {
			if (name_iter.second.info.name == name_string) { 
				fprintf(stderr, "Client with name %s already found. Renaming.\n", name_string.c_str());
				needs_renaming = 1; break; 
			}
		}
		if (needs_renaming) { name_string = name_string + "(" + std::to_string(rename) + ")"; }
		++rename;
		unsigned short new_id = add_client(from, name_string);
		auto newcl_iter = clients.find(new_id);
		if(newcl_iter != clients.end()) {
			handshake(&newcl_iter->second);
		}
		else {
			fprintf(stderr, "server: internal error @ handshake (this shouldn't be happening!).\n");
		}
		post_peer_list();
	}
	else if (cmd == C_CHAT_MESSAGE) {
		fprintf(stderr, "C_CHAT_MESSAGE: %s: %s\n", client_iter->second.info.name.c_str(), socket.get_inbound_buffer() + PTCL_HEADER_LENGTH);
		distribute_chat_message(socket.get_inbound_buffer() + PTCL_HEADER_LENGTH, client_id);
	}
	else if (cmd == C_QUIT) {
		remove_client(client_iter);
		post_client_disconnect(client_id);
		fprintf(stderr, "Received C_QUIT from client %d\n", client_id);
	}
	else {
		fprintf(stderr, "received unrecognized cmdbyte %u with arg %u\n", cmdbyte_arg_mask.ch[0], cmdbyte_arg_mask.ch[1]);
	}

}

void Server::distribute_chat_message(const std::string &msg, const unsigned short sender_id) {
	// using the listen thread, so use socket buffer
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = S_CLIENT_CHAT_MESSAGE;
	cmd_arg_mask.ch[1] = msg.length();
	size_t total_size = PTCL_HEADER_LENGTH;
	protocol_make_header(socket.get_outbound_buffer(), ID_SERVER, 0, cmd_arg_mask.us);
	
	total_size += socket.copy_to_outbound_buffer(&sender_id, sizeof(sender_id), PTCL_HEADER_LENGTH);
	total_size += socket.copy_to_outbound_buffer(msg.c_str(), msg.length(), total_size);
	send_data_to_all(total_size);
}

void Server::post_client_connect(const struct Client &c) {
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = S_CLIENT_CONNECT;
	cmd_arg_mask.ch[1] = 0x00;
	protocol_make_header(socket.get_outbound_buffer(), ID_SERVER, 0, cmd_arg_mask.us);
	char buffer[128];
	int bytes= sprintf_s(buffer, 128, "%u/%s/%s/%u", c.info.id, c.info.name.c_str(), c.info.ip_string.c_str(), c.info.color);
	buffer[bytes-1] = '\0';
	socket.copy_to_outbound_buffer(buffer, bytes, PTCL_HEADER_LENGTH);
	send_data_to_all(PTCL_HEADER_LENGTH + bytes);
}

unsigned short Server::add_client(struct sockaddr_in *newclient_saddr, const std::string &name) {
	++num_clients;
	static uint8_t color = 0;

	struct Client newclient;
	newclient.address = *newclient_saddr;
	newclient.info.id = num_clients;
	newclient.info.name = name;
	newclient.info.color = color;
	newclient.active_ping_seq_number = 0;
	newclient.seq_number = 0;

	color = color > 7 ? 0 : (color+1);

	newclient.info.ip_string = get_dot_notation_ipv4(newclient_saddr);

	clients.insert(std::pair<unsigned short, struct Client>(newclient.info.id, newclient));

	fprintf(stderr, "Server: added client \"%s\" with id %d\n", newclient.info.name.c_str(), newclient.info.id);

	return newclient.info.id;
}


void Server::post_client_disconnect(unsigned short id) {
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = S_CLIENT_DISCONNECT;
	cmd_arg_mask.ch[1] = 0x00;

	protocol_make_header(socket.get_outbound_buffer(), ID_SERVER, 0, cmd_arg_mask.us); 
	socket.copy_to_outbound_buffer(&id, sizeof(id), PTCL_HEADER_LENGTH);

	send_data_to_all(PTCL_HEADER_LENGTH + sizeof(id));
}

void Server::send_data_to_all(size_t size) {

	for (auto &it : clients) {
		struct Client &c = it.second;
		protocol_update_seq_number(socket.get_outbound_buffer(), c.seq_number);
		send_data_to_client(c, size);
	}

}

void Server::send_data_to_all(char* data, size_t size) {

	for (auto &it : clients) {
		struct Client &c = it.second;
		protocol_update_seq_number((char*)data, c.seq_number);
		send_data_to_client(c, data, size);
	}
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

void Server::handshake(struct Client *client) {

	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = S_HANDSHAKE_OK;
	cmd_arg_mask.ch[1] = client->info.color;

	protocol_make_header(socket.get_outbound_buffer(), client->info.id, client->seq_number, cmd_arg_mask.us);
	unsigned total_size = PTCL_HEADER_LENGTH;

	socket.copy_to_outbound_buffer(&client->info.id, sizeof(client->info.id), total_size);
	total_size += sizeof(client->info.id);

	socket.copy_to_outbound_buffer(client->info.name.c_str(), client->info.name.length(), total_size);
	total_size += client->info.name.length();
	int bytes = send_data_to_client(*client, total_size);
}

void Server::increment_client_seq_number(struct Client &c) {
	++c.seq_number;
}

int Server::send_data_to_client(struct Client &client, size_t data_size) {
	// these wrappers around the raw socket::send_data are used to increment sequence numbers appropriately
	if (data_size > PACKET_SIZE_MAX) {
		fprintf(stderr, "send_data_to_client: WARNING! data_size > PACKET_SIZE_MAX. Truncating -> errors inbound.\n");
		data_size = PACKET_SIZE_MAX;
	}
	int bytes = socket.send_data(&client.address, data_size);
	increment_client_seq_number(client);
	return bytes;
}


int Server::send_data_to_client(struct Client &client, const char* buffer, size_t data_size) {
	if (data_size > PACKET_SIZE_MAX) {
		fprintf(stderr, "send_data_to_client: WARNING! data_size > PACKET_SIZE_MAX. Truncating -> errors inbound.\n");
		data_size = PACKET_SIZE_MAX;
	}
	int bytes = socket.send_data(&client.address, (const void*)buffer, data_size);
	increment_client_seq_number(client);
	return bytes;
}

// this is horrible, horrible code!
static char ping_buffer[PACKET_SIZE_MAX];

void Server::ping_client(struct Client &c) {
	//fprintf(stderr, "Sending S_PING to client %d. (seq = %d)\n", c.info.id, c.seq_number);
	c.active_ping_seq_number = c.seq_number;
	protocol_update_seq_number(ping_buffer, c.seq_number);
	send_data_to_client(c, ping_buffer, PTCL_HEADER_LENGTH);
}

void Server::ping_clients() {

	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = S_PING;
	protocol_make_header(ping_buffer, ID_SERVER, 0, cmd_arg_mask.us);

#define PING_SOFT_TIMEOUT_MS 1000
#define PING_HARD_TIMEOUT_MS 5000
#define PING_GRANULARITY_MS 1000

	static _timer ping_timer;

	while (listening) {
		ping_timer.begin();
		if (clients.size() < 1) { Sleep(250); }
		id_client_map::iterator iter = clients.begin();
		while (iter != clients.end()) {

			struct Client &c = iter->second;
			unsigned milliseconds_passed = c.ping_timer.get_ms();

		// this is assuming 100% packet transmittance, which for UDP is... well.. pretty tight
			if (c.active_ping_seq_number == 0) { // no ping request active, make a new one :P
				ping_client(c);
				c.ping_timer.begin();
				++iter;
			}

			else if (milliseconds_passed > PING_SOFT_TIMEOUT_MS) {
				if (milliseconds_passed > PING_HARD_TIMEOUT_MS) {
					fprintf(stderr, "client %u: unanswered S_PING request for more than %d seconds(s) (hard timeout), kicking.\n", c.info.id, PING_HARD_TIMEOUT_MS);
					iter = remove_client(iter);
				}
				else {
					fprintf(stderr, "client %u: unanswered S_PING request for more than %d second(s) (soft timeout), repinging.\n", c.info.id, PING_SOFT_TIMEOUT_MS);
					ping_client(c);
					++iter;
				}

			}
			else {
				++iter;
			}
	
		}
		long wait = PING_GRANULARITY_MS - ping_timer.get_ms();
		if (wait > 1) { Sleep(wait); }
	}

}

void Server::post_peer_list() {
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = S_PEER_LIST;
	cmd_arg_mask.ch[1] = (unsigned char) clients.size();
	protocol_make_header(socket.get_outbound_buffer(), ID_SERVER, 0, cmd_arg_mask.us);

	static const size_t buf_size = PACKET_SIZE_MAX-PTCL_HEADER_LENGTH;
	char peer_buf[512];	// FIXME: HORRIBLE WORKAROUND (intermittent stack corruption errors at sprintf_s)
	size_t offset = 0;

	id_client_map::iterator iter = clients.begin();
	while (iter != clients.end()) {
		struct Client &c = iter->second;

		int bytes = sprintf_s(peer_buf + offset, buf_size, "|%u/%s/%s/%u", c.info.id, c.info.name.c_str(), c.info.ip_string.c_str(), c.info.color);
		offset += bytes;
		if (offset >= buf_size - 1) { fprintf(stderr, "post_peer_list: warning: offset > max\n"); offset = buf_size - 1; break; }
		++iter;
	}
	peer_buf[offset] = '\0';
	socket.copy_to_outbound_buffer(peer_buf, offset, PTCL_HEADER_LENGTH);

	fprintf(stderr, "Server: sending peer list to all clients.\n");

	send_data_to_all(PTCL_HEADER_LENGTH + offset);
}

void Server::broadcast_state() {
	static char broadcast_buffer[PACKET_SIZE_MAX];
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = S_POSITION_UPDATE;
	cmd_arg_mask.ch[1] = (unsigned char) clients.size();

	protocol_make_header(broadcast_buffer, ID_SERVER, 0, cmd_arg_mask.us);
	size_t total_size = PTCL_HEADER_LENGTH;
	for (auto &it : clients) {
		// construct packet to be distributed
		const struct Client &c = it.second;
		memcpy(broadcast_buffer + total_size, &c.info.id, sizeof(c.info.id));
		total_size += sizeof(c.info.id);

		memcpy(broadcast_buffer + total_size, &c.car.data_serial, sizeof(c.car.data_serial));
		total_size += sizeof(c.car.data_serial);
	}

	send_data_to_all(broadcast_buffer, total_size);
}

static inline void calculate_state_client(struct Client &c) {

	static const float fwd_modifier = 0.008;
	static const float side_modifier = 0.005;
	static const float mouse_modifier = 0.0004;

	static const float turning_modifier_forward = 0.19;
	static const float turning_modifier_reverse = 0.16;

	static const float accel_modifier = 0.012;
	static const float brake_modifier = 0.010;
	float car_acceleration = 0.0;

	float car_prev_velocity;
	car_prev_velocity = c.car.data_internal.velocity;

	if (c.keystate & C_KEYSTATE_UP) {
		c.car.data_internal.velocity += accel_modifier;
	}
	else {
		c.car.data_symbolic.susp_angle_roll *= 0.90;
		c.car.data_internal.front_wheel_tmpx *= 0.50;
	}

	c.car.data_symbolic.wheel_rot -= 1.05*c.car.data_internal.velocity;

	if (c.keystate & C_KEYSTATE_DOWN) {
		if (c.car.data_internal.velocity > 0.01) {
			c.car.data_symbolic.direction -= 3*c.car.data_internal.F_centripetal*c.car.data_internal.velocity*0.20;
			c.car.data_internal.velocity *= 0.99;
		}
		c.car.data_internal.velocity -= brake_modifier;

	}
	else {
		c.car.data_symbolic.susp_angle_roll *= 0.90;
		c.car.data_internal.front_wheel_tmpx *= 0.50;
	}	
	c.car.data_internal.velocity *= 0.95;	// regardless of keystate

	if (c.keystate & C_KEYSTATE_LEFT) {
		if (c.car.data_internal.front_wheel_tmpx < 15.0) {
			c.car.data_internal.front_wheel_tmpx += 0.5;
		}
		c.car.data_internal.F_centripetal = -1.0;
		c.car.data_symbolic.front_wheel_angle = f_wheel_angle(c.car.data_internal.front_wheel_tmpx);
		c.car.data_symbolic.susp_angle_roll = fabs(c.car.data_symbolic.front_wheel_angle*c.car.data_internal.velocity*0.8);
		if (c.car.data_internal.velocity > 0) {
			c.car.data_symbolic.direction -= turning_modifier_forward*c.car.data_internal.F_centripetal*c.car.data_internal.velocity;
		}
		else {
			c.car.data_symbolic.direction -= turning_modifier_reverse*c.car.data_internal.F_centripetal*c.car.data_internal.velocity;
		}

	} 
	if (c.keystate & C_KEYSTATE_RIGHT) {
		if (c.car.data_internal.front_wheel_tmpx > -15.0) {
			c.car.data_internal.front_wheel_tmpx -= 0.5;
		}
		c.car.data_internal.F_centripetal = 1.0;
		c.car.data_symbolic.front_wheel_angle = f_wheel_angle(c.car.data_internal.front_wheel_tmpx);
		c.car.data_symbolic.susp_angle_roll = -fabs(c.car.data_symbolic.front_wheel_angle*c.car.data_internal.velocity*0.8);
		if (c.car.data_internal.velocity > 0) {
			c.car.data_symbolic.direction -= turning_modifier_forward*c.car.data_internal.F_centripetal*c.car.data_internal.velocity;
		}
		else {
			c.car.data_symbolic.direction -= turning_modifier_reverse*c.car.data_internal.F_centripetal*c.car.data_internal.velocity;
		}

	} 

	car_prev_velocity = 0.5*(c.car.data_internal.velocity+car_prev_velocity);
	car_acceleration = 0.2*(c.car.data_internal.velocity - car_prev_velocity) + 0.8*car_acceleration;

	if (!(c.keystate & C_KEYSTATE_LEFT) && !(c.keystate & C_KEYSTATE_RIGHT)){
		c.car.data_internal.front_wheel_tmpx *= 0.30;
		c.car.data_symbolic.front_wheel_angle = f_wheel_angle(c.car.data_internal.front_wheel_tmpx);
		c.car.data_symbolic.susp_angle_roll *= 0.50;
		c.car.data_symbolic.susp_angle_fwd *= 0.50;
		c.car.data_internal.F_centripetal = 0.0;
	}

	c.car.data_symbolic.susp_angle_fwd = 7*car_acceleration;
	c.car.data_symbolic._position[0] += c.car.data_internal.velocity*sin(c.car.data_symbolic.direction-M_PI/2);
	c.car.data_symbolic._position[2] += c.car.data_internal.velocity*cos(c.car.data_symbolic.direction-M_PI/2);
}

void Server::calculate_state() {
	
#define POSITION_UPDATE_GRANULARITY_MS 16
	static _timer calculate_timer;
	calculate_timer.begin();
	while (listening) {
		if (clients.size() <= 0) { Sleep(250); }
		else if (calculate_timer.get_ms() > POSITION_UPDATE_GRANULARITY_MS) {
			for (auto &it : clients) { calculate_state_client(it.second); }
			//fprintf(stderr, "Broadcasting game status to all clients.\n");
			broadcast_state();
			calculate_timer.begin();
		}
		long wait = POSITION_UPDATE_GRANULARITY_MS - calculate_timer.get_ms();
		if (wait > 0) { Sleep(wait); }
	}
}


void Server::broadcast_shutdown_message() {
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = S_SHUTDOWN;
	cmd_arg_mask.ch[1] = 0x00;
	protocol_make_header(socket.get_outbound_buffer(), ID_SERVER, 0, cmd_arg_mask.us);

	send_data_to_all(PTCL_HEADER_LENGTH);
}



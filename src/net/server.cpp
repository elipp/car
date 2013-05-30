#include "net/server.h"

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
	_timer ping_timer;
	ping_timer.begin();

	_timer position_update_timer;
	position_update_timer.begin();
	
	while(listening) {
		sockaddr_in from;
		int bytes = socket.receive_data(&from);
		if (bytes > 0) {
			Server::handle_current_packet(&from);
		}
		#define PING_GRANULARITY_MS 1000
		if (ping_timer.get_ms() > PING_GRANULARITY_MS) {
			ping_clients();
			ping_timer.begin();
		}
		#define POSITION_UPDATE_GRANULARITY_MS 16
		if (position_update_timer.get_ms() > POSITION_UPDATE_GRANULARITY_MS) {
			calculate_state();
			broadcast_state();
			position_update_timer.begin();
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
	
	if (cmd == C_KEYSTATE) {
		auto it = clients.find(client_id);
		if (it != clients.end()) {
			//if (seq > it->second.seq_number) {
				it->second.keystate = cmdbyte_arg_mask.ch[1];
			//}
		}
		else {
			fprintf(stderr, "received keystate from unknown client %u, ignoring\n", it->second.info.id);
		}
	}

	else if (cmd == C_PONG) {
		auto client_iter = clients.find(client_id);
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
	else if (cmd == C_QUIT) {
		remove_client(clients.find(client_id));
		post_client_disconnect(client_id);
		fprintf(stderr, "Received C_QUIT from client %d\n", client_id);
	}
	else {
		fprintf(stderr, "received unrecognized cmdbyte %u with arg %u\n", cmdbyte_arg_mask.ch[0], cmdbyte_arg_mask.ch[1]);
	}

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

int Server::send_data_to_client(struct Client &client, size_t data_size) {
	// use this wrapper in order to appropriately increment seq numberz
	if (data_size > PACKET_SIZE_MAX) {
		fprintf(stderr, "send_data_to_client: WARNING! data_size > PACKET_SIZE_MAX. Truncating -> errors inbound.\n");
		data_size = PACKET_SIZE_MAX;
	}
	int bytes = socket.send_data(&client.address, data_size);
	++client.seq_number;
	return bytes;
}

void Server::ping_client(struct Client &c) {
		//fprintf(stderr, "Sending S_PING to client %d. (seq = %d)\n", c.info.id, c.seq_number);
		c.active_ping_seq_number = c.seq_number;
		protocol_update_seq_number(socket.get_outbound_buffer(), c.seq_number);
		send_data_to_client(c, PTCL_HEADER_LENGTH);
}

void Server::ping_clients() {
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = S_PING;
	protocol_make_header(socket.get_outbound_buffer(), ID_SERVER, 0, cmd_arg_mask.us);
	id_client_map::iterator iter = clients.begin();
	while (iter != clients.end()) {
		
		struct Client &c = iter->second;
		unsigned seconds_passed = c.ping_timer.get_s();

		// this is assuming 100% packet transmittance, which for UDP is... well.. pretty tight
		if (c.active_ping_seq_number == 0) { // no ping request active, make a new one :P
			ping_client(c);
			c.ping_timer.begin();
			++iter;
		}
		#define PING_SOFT_TIMEOUT_S 1
		#define PING_HARD_TIMEOUT_S 5
		else if (seconds_passed > PING_SOFT_TIMEOUT_S) {
				if (seconds_passed > PING_HARD_TIMEOUT_S) {
					fprintf(stderr, "client %u: unanswered S_PING request for more than %d seconds(s) (hard timeout), kicking.\n", c.info.id, PING_HARD_TIMEOUT_S);
					iter = remove_client(iter);
				}
				else {
					fprintf(stderr, "client %u: unanswered S_PING request for more than %d second(s) (soft timeout), repinging.\n", c.info.id, PING_SOFT_TIMEOUT_S);
					ping_client(c);
				}
		
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
	
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = S_POSITION_UPDATE;
	cmd_arg_mask.ch[1] = (unsigned char) clients.size();

	protocol_make_header(socket.get_outbound_buffer(), ID_SERVER, 0, cmd_arg_mask.us);
	size_t total_size = PTCL_HEADER_LENGTH;
	for (auto &it : clients) {
		// construct packet to be distributed
		const struct Client &c = it.second;
		socket.copy_to_outbound_buffer(&c.info.id, sizeof(c.info.id), total_size);
		total_size += sizeof(c.info.id);
		
		socket.copy_to_outbound_buffer(&c.car, sizeof(c.car), total_size);
		total_size += sizeof(c.car);
	}

	send_data_to_all(total_size);
}

void Server::calculate_state() {

	static const float fwd_modifier = 0.008;
	static const float side_modifier = 0.005;
	static const float mouse_modifier = 0.0004;

	static const float turning_modifier_forward = 0.19;
	static const float turning_modifier_reverse = 0.16;

	static const float accel_modifier = 0.012;
	static const float brake_modifier = 0.010;

	for (auto &it : clients) {
		struct Client &c = it.second;
		float car_acceleration = 0.0;
		float car_prev_velocity;
		car_prev_velocity = c.car.velocity;

		if (c.keystate & C_KEYSTATE_UP) {
			c.car.velocity += accel_modifier;
		}
		else {
			c.car.susp_angle_roll *= 0.90;
			c.car.front_wheel_tmpx *= 0.50;
		}

		c.car.wheel_rot -= 1.05*c.car.velocity;
		
		if (c.keystate & C_KEYSTATE_DOWN) {
			if (c.car.velocity > 0.01) {
				c.car.direction -= 3*c.car.F_centripetal*c.car.velocity*0.20;
				c.car.velocity *= 0.99;
			}
			c.car.velocity -= brake_modifier;

		}
		else {
				c.car.susp_angle_roll *= 0.90;
				c.car.front_wheel_tmpx *= 0.50;
		}
		c.car.velocity *= 0.95;	// regardless of keystate

		if (c.keystate & C_KEYSTATE_LEFT) {
			if (c.car.front_wheel_tmpx < 15.0) {
				c.car.front_wheel_tmpx += 0.5;
			}
			c.car.F_centripetal = -1.0;
			c.car.front_wheel_angle = f_wheel_angle(c.car.front_wheel_tmpx);
			c.car.susp_angle_roll = fabs(c.car.front_wheel_angle*c.car.velocity*0.8);
			if (c.car.velocity > 0) {
				c.car.direction -= turning_modifier_forward*c.car.F_centripetal*c.car.velocity;
			}
			else {
				c.car.direction -= turning_modifier_reverse*c.car.F_centripetal*c.car.velocity;
			}

		} 
		if (c.keystate & C_KEYSTATE_RIGHT) {
			if (c.car.front_wheel_tmpx > -15.0) {
				c.car.front_wheel_tmpx -= 0.5;
			}
			c.car.F_centripetal = 1.0;
			c.car.front_wheel_angle = f_wheel_angle(c.car.front_wheel_tmpx);
			c.car.susp_angle_roll = -fabs(c.car.front_wheel_angle*c.car.velocity*0.8);
			if (c.car.velocity > 0) {
				c.car.direction -= turning_modifier_forward*c.car.F_centripetal*c.car.velocity;
			}
			else {
				c.car.direction -= turning_modifier_reverse*c.car.F_centripetal*c.car.velocity;
			}

		} 

		car_prev_velocity = 0.5*(c.car.velocity+car_prev_velocity);
		car_acceleration = 0.2*(c.car.velocity - car_prev_velocity) + 0.8*car_acceleration;

		if (!(c.keystate & C_KEYSTATE_LEFT) && !(c.keystate & C_KEYSTATE_RIGHT)){
			c.car.front_wheel_tmpx *= 0.30;
			c.car.front_wheel_angle = f_wheel_angle(c.car.front_wheel_tmpx);
			c.car.susp_angle_roll *= 0.50;
			c.car.susp_angle_fwd *= 0.50;
			c.car.F_centripetal = 0.0;
		}
		
		c.car.susp_angle_fwd = 7*car_acceleration;
		c.car._position[0] += c.car.velocity*sin(c.car.direction-M_PI/2);
		c.car._position[2] += c.car.velocity*cos(c.car.direction-M_PI/2);
		//onScreenLog::print( "car %u: pos: ", c.info.id);
		//c.car.position().print();
	}
}

void Server::broadcast_shutdown_message() {
	command_arg_mask_union cmd_arg_mask;
	cmd_arg_mask.ch[0] = S_SHUTDOWN;
	cmd_arg_mask.ch[1] = 0x00;
	protocol_make_header(socket.get_outbound_buffer(), ID_SERVER, 0, cmd_arg_mask.us);
	
	send_data_to_all(PTCL_HEADER_LENGTH);
}



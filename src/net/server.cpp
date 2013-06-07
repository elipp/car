#include "net/server.h"
#include "net/client.h"

#ifdef SERVER_CLI
#define SERVER_PRINT(fmt, ... ) fprintf(stderr, fmt, __VA_ARGS__)
#else
#include "net/client.h"
#include "text.h"
#define SERVER_PRINT(fmt, ...) onScreenLog::print(fmt, __VA_ARGS__)
#endif

#include <cmath>


unsigned Server::seq_number = 0;

Server::Listen Server::Listener;
Server::Ping Server::PingManager;
Server::GameState Server::GameStateManager;

std::unordered_map<unsigned short, struct Client> Server::clients;
unsigned Server::num_clients = 0;
int Server::_running = 0;
Socket Server::socket;

void _NETTASKTHREAD_CALLBACK Server::listen_task() {
	Listener.listen_loop();
}

 void Server::Listen::listen_loop(void) {
	
	while(thread.running()) {
		sockaddr_in from;
		int bytes = socket.receive_data(thread.buffer, &from);
	
		if (bytes > 0) {
			handle_current_packet(&from);
		}
	}
	broadcast_shutdown_message();
	SERVER_PRINT("Stopping listen.\n");
}

int Server::init(unsigned short port) {
	Socket::initialize();
	socket = Socket(port, SOCK_DGRAM, true);	// for UDP and non-blocking
	if (socket.bad()) { SERVER_PRINT( "Server::init, socket init error.\n"); return 0; }
	
	Listener.start();
	PingManager.start();
	GameStateManager.start();
	
	_running = 1;
	SERVER_PRINT("Server: init successful. Bound to port %u.\n", get_port());
	return 1;
}

void Server::shutdown() {
	// post quit messages to all clients
	_running = 0;
	SERVER_PRINT( "Server: shutting down. Broadcasting S_SHUTDOWN.\n");
	SERVER_PRINT( "Server: joining worker threads.\n");
	
	if (clients.size() <= 0) {
		socket.close();
		Socket::deinitialize();	// this quickly breaks up the listen loop, no need to send S_SHUTDOWN either
	}

	GameStateManager.stop();
	PingManager.stop();
	Listener.stop();
	
	clients.clear();
	socket.close();
}


void Server::Listen::handle_current_packet(_OUT struct sockaddr_in *from) {
	
	PTCLHEADERDATA header;
	protocol_get_header_data(thread.buffer, &header);
	
	if (header.protocol_id != PROTOCOL_ID) {
		SERVER_PRINT( "dropping packet. Reason: protocol_id mismatch (%x)!\nDump:\n", header.protocol_id);
		//buffer_print_raw(socket.get_inbound_buffer(), socket.current_data_length_in());
		return;
	}
	
	auto client_iter = clients.find(header.sender_id);

	if (header.sender_id != ID_CLIENT_UNASSIGNED) {
		if (client_iter == clients.end()) {
			SERVER_PRINT( "Server: warning: received packet from non-existing player-id %u. Ignoring.\n", header.sender_id);
			return;
		}
	}

	const unsigned char &cmd = header.cmd_arg_mask.ch[0];
	const unsigned char &cmd_arg = header.cmd_arg_mask.ch[1];

	std::string ip = get_dot_notation_ipv4(from);
	//SERVER_PRINT( "Received packet with size %u from %s (id = %u). Seq_number = %u\n", socket.current_data_length_in(), ip.c_str(), client_id, seq);

	
	if (cmd == C_KEYSTATE) {
		if (client_iter != clients.end()) {
			//if (seq > it->second.seq_number) {
			client_iter->second.keystate = cmd_arg;
			//}
		}
		else {
			SERVER_PRINT( "received keystate from unknown client %u, ignoring\n", client_iter->second.info.id);
		}
	}

	else if (cmd == C_PONG) {
		if (client_iter != clients.end()) {
			struct Client &c = client_iter->second;
			unsigned int embedded_pong_seq_number;
			thread.copy_from_buffer(&embedded_pong_seq_number, sizeof(unsigned int), PTCL_HEADER_LENGTH);
			if (embedded_pong_seq_number == c.active_ping_seq_number) {
				//SERVER_PRINT( "Received C_PONG from client %d with seq_number %d (took %d us)\n", c.info.id, embedded_pong_seq_number, (int)c.ping_timer.get_us());
				c.active_ping_seq_number = 0;
			}
			else {
				SERVER_PRINT( "Received C_PONG from client %d, but the seq_number embedded in the datagram (%d, bytes 12-16) doesn't match expected seq_number (%d)!\n", embedded_pong_seq_number, c.active_ping_seq_number);
			}
		}
		else {
			SERVER_PRINT( "Server: error: couldn't find client with id %d (this shouldn't be happening!)\n", header.sender_id);
		}
	}

	else if (cmd == C_HANDSHAKE) {
		std::string name_string(thread.buffer + PTCL_HEADER_LENGTH);
		static unsigned rename = 0;
		int needs_renaming = 0;
		for (auto &name_iter : clients) {
			if (name_iter.second.info.name == name_string) { 
				SERVER_PRINT( "Client with name %s already found. Renaming.\n", name_string.c_str());
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
			SERVER_PRINT( "Server: internal error @ handshake (this shouldn't be happening!).\n");
		}
		post_peer_list();
	}
	else if (cmd == C_CHAT_MESSAGE) {
		const std::string msg(thread.buffer + PTCL_HEADER_LENGTH);
		SERVER_PRINT( "%s: %s\n", client_iter->second.info.name.c_str(), msg.c_str());
		distribute_chat_message(msg, header.sender_id);
	}
	else if (cmd == C_QUIT) {
		remove_client(client_iter);
		post_client_disconnect(header.sender_id);
		SERVER_PRINT( "Server: received C_QUIT from client %d\n", header.sender_id);
	}
	else {
		SERVER_PRINT( "warning: received unrecognized cmdbyte %u with arg %u\n", cmd, cmd_arg);
	}

}

void Server::Listen::distribute_chat_message(const std::string &msg, const unsigned short sender_id) {

	PTCLHEADERDATA CHAT_MESSAGE_HEADER = { PROTOCOL_ID, SEQN_ASSIGNED_ELSEWHERE, ID_SERVER, S_CLIENT_CHAT_MESSAGE };
	CHAT_MESSAGE_HEADER.cmd_arg_mask.ch[1] = (unsigned char)msg.length();

	size_t accum_offset = thread.copy_to_buffer(&CHAT_MESSAGE_HEADER, sizeof(CHAT_MESSAGE_HEADER), 0);
	
	accum_offset += thread.copy_to_buffer(&sender_id, sizeof(sender_id), accum_offset);
	accum_offset += thread.copy_to_buffer(msg.c_str(), msg.length(), accum_offset);
	send_data_to_all(thread.buffer, accum_offset);
}

void Server::Listen::post_client_connect(const struct Client &c) {
	PTCLHEADERDATA CLIENT_CONNECT_HEADER = { PROTOCOL_ID, SEQN_ASSIGNED_ELSEWHERE, ID_SERVER, S_CLIENT_CONNECT };
	thread.copy_to_buffer(&CLIENT_CONNECT_HEADER, sizeof(CLIENT_CONNECT_HEADER), 0);
	char buffer[128];
	int bytes= sprintf_s(buffer, 128, "%u/%s/%s/%u", c.info.id, c.info.name.c_str(), c.info.ip_string.c_str(), c.info.color);
	buffer[bytes-1] = '\0';
	thread.copy_to_buffer(buffer, bytes, PTCL_HEADER_LENGTH);
	send_data_to_all(thread.buffer, PTCL_HEADER_LENGTH + bytes);
}

unsigned short Server::Listen::add_client(struct sockaddr_in *newclient_saddr, const std::string &name) {
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

	SERVER_PRINT( "Server: added client \"%s\" with id %d\n", newclient.info.name.c_str(), newclient.info.id);

	return newclient.info.id;
}


void Server::Listen::post_client_disconnect(unsigned short id) {
	
	PTCLHEADERDATA CLIENT_DISCONNECT_HEADER = { PROTOCOL_ID, SEQN_ASSIGNED_ELSEWHERE, ID_SERVER, S_CLIENT_DISCONNECT };
	thread.copy_to_buffer(&CLIENT_DISCONNECT_HEADER, sizeof(CLIENT_DISCONNECT_HEADER), 0); 
	thread.copy_to_buffer(&id, sizeof(id), PTCL_HEADER_LENGTH);

	send_data_to_all(thread.buffer, PTCL_HEADER_LENGTH + sizeof(id));
}

void Server::send_data_to_all(char *buffer, size_t size) {

	for (auto &it : clients) {
		struct Client &c = it.second;
		protocol_update_seq_number(buffer, c.seq_number);
		send_data_to_client(c, buffer, size);
	}

}

id_client_map::iterator Server::remove_client(id_client_map::iterator &iter) {
	id_client_map::iterator it;
	if (iter != clients.end()) {
		it = clients.erase(iter);
		return it;
	}
	else {
		SERVER_PRINT( "Server: remove client: internal error: couldn't find client %u from the client_id map (this shouldn't be happening)!\n");
		return clients.end();
	}
}

void Server::Listen::handshake(struct Client *client) {

	PTCLHEADERDATA HANDSHAKE_HEADER = 
		protocol_make_header( SEQN_ASSIGNED_ELSEWHERE, ID_SERVER, S_HANDSHAKE_OK, client->info.color);
	
	int accum_offset = protocol_copy_header(thread.buffer, &HANDSHAKE_HEADER);
	accum_offset += thread.copy_to_buffer(&client->info.id, sizeof(client->info.id), accum_offset);
	accum_offset += thread.copy_to_buffer(client->info.name.c_str(), client->info.name.length(), accum_offset);
	send_data_to_client(*client, thread.buffer, accum_offset);
}

void Server::Listen::post_peer_list() {

	PTCLHEADERDATA PEER_LIST_HEADER = 
		protocol_make_header( SEQN_ASSIGNED_ELSEWHERE, ID_SERVER, S_PEER_LIST, clients.size());
	protocol_copy_header(thread.buffer, &PEER_LIST_HEADER);
	
	static const size_t buf_size = PACKET_SIZE_MAX-PTCL_HEADER_LENGTH;
	char peer_buf[512];	// FIXME: HORRIBLE WORKAROUND (fix intermittent stack corruption errors at sprintf_s)
	size_t offset = 0;

	id_client_map::iterator iter = clients.begin();
	while (iter != clients.end()) {
		struct Client &c = iter->second;

		int bytes = sprintf_s(peer_buf + offset, buf_size, "|%u/%s/%s/%u", c.info.id, c.info.name.c_str(), c.info.ip_string.c_str(), c.info.color);
		offset += bytes;
		if (offset >= buf_size - 1) { SERVER_PRINT( "post_peer_list: warning: offset > max\n"); offset = buf_size - 1; break; }
		++iter;
	}
	peer_buf[offset] = '\0';
	thread.copy_to_buffer(peer_buf, offset, PTCL_HEADER_LENGTH);

	SERVER_PRINT( "Server: sending peer list to all clients.\n");

	send_data_to_all(thread.buffer, PTCL_HEADER_LENGTH + offset);
}

void Server::Listen::broadcast_shutdown_message() {
	PTCLHEADERDATA SHUTDOWN_HEADER = protocol_make_header( SEQN_ASSIGNED_ELSEWHERE, ID_SERVER, S_SHUTDOWN); 
	protocol_copy_header(thread.buffer, &SHUTDOWN_HEADER);
	send_data_to_all(thread.buffer, PTCL_HEADER_LENGTH);
}
void Server::Ping::ping_client(struct Client &c) {
	//SERVER_PRINT( "Sending S_PING to client %d. (seq = %d)\n", c.info.id, c.seq_number);
	// THIS IS ASSUMING THE PROTOCOL HEADER HAS BEEN PROPERLY SETUP.
	c.active_ping_seq_number = c.seq_number;
	protocol_update_seq_number(thread.buffer, c.seq_number);
	send_data_to_client(c, thread.buffer, PTCL_HEADER_LENGTH);
}
void _NETTASKTHREAD_CALLBACK Server::ping_task() {
	PingManager.ping_loop();
}

void Server::Ping::ping_loop() {

	PTCLHEADERDATA PING_HEADER = protocol_make_header( SEQN_ASSIGNED_ELSEWHERE, ID_SERVER, S_PING);
	protocol_copy_header(thread.buffer, &PING_HEADER);

#define PING_SOFT_TIMEOUT_MS 1000
#define PING_HARD_TIMEOUT_MS 5000
#define PING_GRANULARITY_MS 1000

	static _timer ping_timer;

	while (thread.running()) {
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
					SERVER_PRINT( "client %u: unanswered S_PING request for more than %d ms (hard timeout), kicking.\n", c.info.id, PING_HARD_TIMEOUT_MS);
					iter = remove_client(iter);
				}
				else {
					SERVER_PRINT( "client %u: unanswered S_PING request for more than %d ms (soft timeout), repinging.\n", c.info.id, PING_SOFT_TIMEOUT_MS);
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


void _NETTASKTHREAD_CALLBACK Server::state_task() {
	GameStateManager.state_loop();
}



void Server::GameState::state_loop() {
	static _timer calculate_timer;
	calculate_timer.begin();

	while(thread.running()) {
		if (clients.size() <= 0) { Sleep(250); }
		else if (calculate_timer.get_ms() > POSITION_UPDATE_GRANULARITY_MS) {
			for (auto &it : clients) { 
				calculate_state_client(it.second); 
			}
			broadcast_state();
			long wait = POSITION_UPDATE_GRANULARITY_MS - calculate_timer.get_ms();
			if (wait > 0) { Sleep(wait); }
			calculate_timer.begin();
		}
	}
}


void Server::GameState::broadcast_state() {
	PTCLHEADERDATA POSITION_UPDATE_HEADER = 
		protocol_make_header( SEQN_ASSIGNED_ELSEWHERE, ID_SERVER, S_POSITION_UPDATE, clients.size());

	protocol_copy_header(thread.buffer, &POSITION_UPDATE_HEADER);
	size_t total_size = PTCL_HEADER_LENGTH;
	for (auto &it : clients) {
		// construct packet to be distributed
		const struct Client &c = it.second;
		total_size += thread.copy_to_buffer(&c.info.id, sizeof(c.info.id), total_size);
		total_size += thread.copy_to_buffer(&c.car.data_serial, sizeof(c.car.data_serial), total_size);

	}
	send_data_to_all(thread.buffer, total_size);
}

void Server::GameState::calculate_state_client(struct Client &c) {
	
	Car &car = c.car;

	float car_acceleration = 0.0;
	float car_prev_velocity = car.state.velocity;
	
	car.state.velocity *= velocity_dissipation;	// regardless of throttle/brake keystate
	car.data_internal.front_wheel_tmpx *= tmpx_dissipation;

	if (c.keystate & C_KEYSTATE_UP) { car.state.velocity += accel_modifier; }

	if (c.keystate & C_KEYSTATE_DOWN) { car.state.velocity -= brake_modifier; }

	if (c.keystate & C_KEYSTATE_LEFT) {
		car.data_internal.front_wheel_tmpx = min(car.data_internal.front_wheel_tmpx+tmpx_modifier, tmpx_limit);
	} 
	if (c.keystate & C_KEYSTATE_RIGHT) {
		car.data_internal.front_wheel_tmpx = max(car.data_internal.front_wheel_tmpx-tmpx_modifier, -tmpx_limit);
	}
	if (!(c.keystate & C_KEYSTATE_LEFT) && !(c.keystate & C_KEYSTATE_RIGHT)){
		car.data_internal.front_wheel_tmpx *= 0.45;
		car.state.susp_angle_roll *= 0.89;
	}
	car.state.front_wheel_angle = f_wheel_angle(car.data_internal.front_wheel_tmpx);
	
	car.state.susp_angle_roll = car.data_internal.front_wheel_tmpx*fabs(car.state.velocity)*0.2;
	
	float turn_coeff = 1.5*pow(8,-(fabs(car.state.velocity)));
	car.state.direction += (car.state.velocity > 0 ? turning_modifier_forward : turning_modifier_reverse)
							*car.state.front_wheel_angle*car.state.velocity*turn_coeff*POSITION_UPDATE_DT_COEFF;
	
	car_prev_velocity = 0.5*(car.state.velocity+car_prev_velocity);
	car_acceleration = 0.2*(car.state.velocity - car_prev_velocity) + 0.8*car_acceleration;


	car.state.wheel_rot -= 1.07*car.state.velocity * POSITION_UPDATE_DT_COEFF;
	car.data_internal.susp_angle_fwd = 7*car_acceleration;
	car.state._position[0] += car.state.velocity*sin(car.state.direction-M_PI/2) * POSITION_UPDATE_DT_COEFF;
	car.state._position[2] += car.state.velocity*cos(car.state.direction-M_PI/2) * POSITION_UPDATE_DT_COEFF;
}

void Server::increment_client_seq_number(struct Client &c) {
	++c.seq_number;
}

int Server::send_data_to_client(struct Client &client, const char* buffer, size_t data_size) {
	if (data_size > PACKET_SIZE_MAX) {
		SERVER_PRINT( "send_data_to_client: WARNING! data_size > PACKET_SIZE_MAX. Truncating -> errors inbound.\n");
		data_size = PACKET_SIZE_MAX;
	}
	int bytes = socket.send_data(&client.address, buffer, data_size);
	increment_client_seq_number(client);
	return bytes;
}





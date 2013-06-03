#include "net/client_funcs.h"
#include "net/server.h"

const std::unordered_map<std::string, const client_funcptr> funcs = create_func_map();

int help (const std::vector<std::string> &args) {
	onScreenLog::print("List of available commands:\n");
	for (auto &it : funcs) {
		onScreenLog::print(" %s\n", it.first.c_str());
	}
	return 1;
}
int connect(const std::vector<std::string> &args) {
	// if properly formatted, we should find ip_port_string at position 1
	if (LocalClient::connected()) {
		onScreenLog::print("connect: already connected!\n");
		return 0;
	}
	if (args.size() == 1) {
		// use default
		return LocalClient::start("127.0.0.1:50000");
	}

	else if (args.size() != 2) {
		return 0;
	}
	else {
		return LocalClient::start(args[1]);
	}
}

int set_name(const std::vector<std::string> &args) {
	if (args.size() <= 1) {
		return 0;
	}
	else {
		LocalClient::set_name(args[1]);
	}
	return 1;
}

int disconnect(const std::vector<std::string> &args) {
	if (LocalClient::connected()) {	
		LocalClient::stop();
		onScreenLog::print("Disconnected from server.\n");
	} else {
		onScreenLog::print("/disconnect: not connected.\n");
	}

	return 1;
}

int quit(const std::vector<std::string> &args) {
	LocalClient::quit();
	stop_main_loop();
	return 1;
}

int startserver(const std::vector<std::string> &args) {
	if (!Server::running()) {
		return Server::init(50000);
	}
	else {
		onScreenLog::print("Local server already running on port %u! Use /stopserver to stop.\n", Server::get_port());
		return 0;
	}
}

int stopserver(const std::vector<std::string> &args) {
	if (Server::running()) {
		onScreenLog::print("Client: stopping local server.\n");
		Server::shutdown();
	}
	return 1;
}

// because those c++11 initializer lists aren't (fully?) supported
const std::unordered_map<std::string, const client_funcptr> create_func_map() {
	std::unordered_map<std::string, const client_funcptr> funcs;

	funcs.insert(std::pair<std::string, const client_funcptr>("/connect", connect));
	funcs.insert(std::pair<std::string, const client_funcptr>("/disconnect", disconnect));
	funcs.insert(std::pair<std::string, const client_funcptr>("/name", set_name));
	funcs.insert(std::pair<std::string, const client_funcptr>("/quit", quit));
	funcs.insert(std::pair<std::string, const client_funcptr>("/help", help));
	funcs.insert(std::pair<std::string, const client_funcptr>("/startserver", startserver));
	funcs.insert(std::pair<std::string, const client_funcptr>("/stopserver", stopserver));

	return funcs;
}
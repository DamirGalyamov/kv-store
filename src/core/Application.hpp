#pragma once

#include "../command/CommandParser.hpp"
#include "../command/Command.hpp"
#include "../storage/KVStore.hpp"

class Application {
public:
	Application(CommandParser& command_parser, KVStore& store);

	void run();
private:

	void processing_get_result(const GetResult&);
	void processing_status(StoreStatus);
	bool read_command(std::string& buffer);

	CommandParser& command_parser_;
	KVStore& store_;
};
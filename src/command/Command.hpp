#pragma once

#include <string>

enum class CommandType{
	put,
	get,
	remove,
	compact,
	exit,
	invalid
};

struct Command {
	CommandType type = CommandType::invalid;
	std::string key;
	std::string value;
};
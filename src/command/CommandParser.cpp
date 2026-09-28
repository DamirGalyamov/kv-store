#include "CommandParser.hpp"

#include <iostream>
#include <vector>

void CommandParser::skip_spaces (
	std::string::const_iterator& it,
	const std::string::const_iterator& end) const
{
	while (it != end && *it == ' ') {
		++it;
	}
}

CommandType CommandParser::read_command(std::string::const_iterator& it, const std::string::const_iterator& end) const {
	std::string command = "";
	while (it != end && *it != ' ') {
		command += *it;
		++it;
	}
	if (command == "PUT") {
		return CommandType::put;
	}
	else if (command == "GET") {
		return CommandType::get;
	}
	else if (command == "DELETE") {
		return CommandType::remove;
	}
	else if (command == "COMPACT") {
		return CommandType::compact;
	}
	else if (command == "EXIT") {
		return CommandType::exit;
	}
	return CommandType::invalid;
}

bool CommandParser::read_argument(std::string& result, std::string::const_iterator& it, const std::string::const_iterator& end) const {
	if (it == end || *it != ' ') {
		return false;
	}
	skip_spaces(it, end);
	if (it == end || *it != '"') {
		return false;
	}
	++it;
	while (it != end) {
		if (*it == '"') {
			++it;
			return true;
		}
		result.push_back(*it);
		++it;
	}
	return false;
}

Command CommandParser::parse(const std::string& buffer) const{
	
	Command command;
	auto it = buffer.begin();
	command.type = read_command(it, buffer.end());
	switch (command.type) {
	case CommandType::put: {
		if (!read_argument(command.key, it, buffer.end()) ||
			!read_argument(command.value, it, buffer.end()))
		{
			command.type = CommandType::invalid;
			break;
		}

		skip_spaces(it, buffer.end());
		if (it != buffer.end()) {
			command.type = CommandType::invalid;
		}
		break;
	}
	case CommandType::remove:
	case CommandType::get: {
		if (read_argument(command.key, it, buffer.end())) {
			skip_spaces(it, buffer.end());
			if (it != buffer.end()) {
				command.type = CommandType::invalid;
			}
			break;
		}
		command.type = CommandType::invalid;
		break;
	}
	case CommandType::compact:
	case CommandType::exit: {
		skip_spaces(it, buffer.end());
		if (it != buffer.end()) {
			command.type = CommandType::invalid;
		}
		break;
	}
	}
	return command;
	}
}
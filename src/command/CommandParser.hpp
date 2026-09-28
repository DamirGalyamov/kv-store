#pragma once

#include "Command.hpp"

#include <string>

class CommandParser {
public:
	void CommandParser::skip_spaces(
		std::string::const_iterator& it,
		const std::string::const_iterator& end) const;

	CommandType CommandParser::read_command(std::string::const_iterator& it, const std::string::const_iterator& end) const;

	bool CommandParser::read_argument(std::string& result, std::string::const_iterator& it, const std::string::const_iterator& end) const;

	Command parse(const std::string& line) const;
};
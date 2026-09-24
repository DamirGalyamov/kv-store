#pragma once

#include "Command.hpp"

#include <string>

class CommandParser {
public:
	Command parse(const std::string& line) const;
};
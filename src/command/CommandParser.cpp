#include "CommandParser.hpp"

#include <iostream>
#include <vector>

Command CommandParser::parse(const std::string& buffer) const{
	bool new_word = true;
	std::vector<std::string> words;
	std::size_t start;
	for (std::size_t i = 0; i < buffer.size(); ++i) {
		if (new_word) {
			if (buffer[i] != ' ') {
				start = i;
				new_word = false;
			}
		}
		else {
			if (buffer[i] == ' ') {
				words.push_back(buffer.substr(start, i - start));
				new_word = true;
			}
		}
	}
	if (!new_word) {
		words.push_back(buffer.substr(start, buffer.size() - start));
	}
	Command command;
	if (words.empty() || words.size() > 3) {
		command.type = CommandType::invalid;
		return command;
	}
	if (words[0] == "PUT") {
		if (words.size() == 3) {
			command.type = CommandType::put;
			command.key = words[1];
			command.value = words[2];
		}
		else {
			command.type = CommandType::invalid;
		}
	}
	else if (words[0] == "GET") {
		if (words.size() == 2) {
			command.type = CommandType::get;
			command.key = words[1];
		}
		else {
			command.type = CommandType::invalid;
		}
	}
	else if (words[0] == "DELETE") {
		if (words.size() == 2) {
			command.type = CommandType::remove;
			command.key = words[1];
		}
		else {
			command.type = CommandType::invalid;
		}
	}
	else if (words[0] == "COMPACT") {
		if (words.size() == 1) {
			command.type = CommandType::compact;
		}
		else {
			command.type = CommandType::invalid;
		}
	}
	else if (words[0] == "EXIT") {
		if (words.size() == 1) {
			command.type = CommandType::exit;
		}
		else {
			command.type = CommandType::invalid;
		}
	}
	else {
		command.type = CommandType::invalid;
	}
	return command;
}
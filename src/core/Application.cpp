#include "Application.hpp"
#include "../storage/Status.hpp"

#include <vector>
#include <string>
#include <iostream>
#include <optional>

Application::Application(CommandParser& command_parser, KVStore& store) : command_parser_(command_parser), store_(store){}

void Application::run() {
	while (true) {
		std::string buffer = "";
		if (!read_command(buffer)) {
			return;
		}
		Command command = command_parser_.parse(buffer);
		if (command.type == CommandType::invalid) {
			std::cout << "Invalid command\n";
			continue;
		}
		switch (command.type) {
		case CommandType::put: {
			StoreStatus status = store_.put(command.key, command.value);
			processing_status(status);
			break;
		}
		case CommandType::get: {
			GetResult command_result = store_.get(command.key);
			processing_get_result(command_result);
			break;
		}
		case CommandType::remove: {
			StoreStatus status = store_.remove(command.key);
			processing_status(status);
			break;
		}
		case CommandType::compact: {
			StoreStatus status = store_.compact();
			processing_status(status);
			break;
		}
		case CommandType::exit: {
			return;
		}
		}
	}
}

bool Application::read_command(std::string& buffer) {
	return static_cast<bool>(
		std::getline(std::cin, buffer)
		);
}

void Application::processing_get_result(const GetResult& result) {
	switch (result.status) {
	case StoreStatus::ok: {
		std::cout << result.value.value() << "\n";
		break;
	}
	case StoreStatus::not_found:{
		std::cout << "Record with this key was not found" << "\n";
		break;
	}
	case StoreStatus::io_error: {
		std::cout << "Sorry, but some problems arose while working with the database. Please, try again later" << "\n";
		break;
	}
	case StoreStatus::invalid_argument: {
		std::cout << "Invalid argument in response" << "\n";
		break;
	}
	default: {
		std::cout << "Storage data is corrupted\n";
	}
	}
}

void Application::processing_status(StoreStatus status) {
	switch (status) {
	case StoreStatus::ok: {
		std::cout << "OK\n";
		break;
	}
	case StoreStatus::not_found: {
		std::cout << "Record with this key was not found" << "\n";
		break;
	}
	case StoreStatus::io_error: {
		std::cout << "Sorry, but some problems arose while working with the database. Please, try again later" << "\n";
		break;
	}
	case StoreStatus::invalid_argument: {
		std::cout << "Invalid argument in response" << "\n";
		break;
	}
	case StoreStatus::corrupted_data: {
		std::cout << "Storage data is corrupted\n";
		break;
	}
	}
}
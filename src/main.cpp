#include "core/Application.hpp"

#include <iostream>

int main()
{

    std::cout << "kv-store started\n";
    CommandParser parser;
    std::string file_name = "database.txt";
    try {
        KVStore store(file_name);
        Application app(parser, store);
        app.run();
    }
    catch (const std::runtime_error& error) {
        std::cout << error.what() << '\n';
    }
    return 0;
}
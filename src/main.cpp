#include "Server.hpp"
#include <iostream>
#include <string>
#include <stdexcept>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return 1;
    }

    try {
        // Convert port to integer
        int port;
        try {
            port = std::stoi(argv[1]);
            if (port < 1024 || port > 65535) {
                throw std::invalid_argument("Port must be between 1024 and 65535");
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid port number. " << e.what() << std::endl;
            return 1;
        }

        // Get password
        std::string password = argv[2];
        if (password.empty()) {
            std::cerr << "Error: Password cannot be empty" << std::endl;
            return 1;
        }

        // Create and start server
        Server server(port, password);
        server.Start();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
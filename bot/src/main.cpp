#include "../include/Bot.hpp"
#include <iostream>
#include <signal.h>

Bot* g_bot = nullptr;

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ". Shutting down bot..." << std::endl;
    if (g_bot) {
        g_bot->disconnect();
    }
    exit(signum);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 6667 pass123" << std::endl;
        return 1;
    }

    // Register signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

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

        // Create bot instance
        Bot bot("127.0.0.1", port, password);
        g_bot = &bot;

        std::cout << "Starting IRC Bot..." << std::endl;
        std::cout << "Connecting to localhost:" << port << std::endl;

        // Connect to server
        if (!bot.connect()) {
            std::cerr << "Failed to connect to server" << std::endl;
            return 1;
        }

        // Authenticate
        bot.authenticate();

        // Run the bot
        std::cout << "Bot is running. Press Ctrl+C to stop." << std::endl;
        std::cout << "The bot will automatically join #bot channel." << std::endl;
        std::cout << "Available commands: !help, !ping, !pong, !hi, !joke" << std::endl;
        bot.run();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 
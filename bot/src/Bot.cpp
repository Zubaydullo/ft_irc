#include "../include/Bot.hpp"
#include <cstdlib>
#include <ctime>

Bot::Bot(const std::string& server, int port, const std::string& password)
    : _socket(-1), _server(server), _port(port), _password(password),
      _nickname("ircbot"), _username("bot"), _realname("IRC Bot"),
      _connected(false), _authenticated(false) {
    initializeJokes();
    srand(time(NULL));
}

Bot::~Bot() {
    disconnect();
}

void Bot::initializeJokes() {
    _jokes.push_back("Why don't programmers like nature? It has too many bugs!");
    _jokes.push_back("How many programmers does it take to change a light bulb? None, that's a hardware problem!");
    _jokes.push_back("Why do Java developers wear glasses? Because they can't C#!");
    _jokes.push_back("What's a programmer's favorite hangout place? Foo Bar!");
    _jokes.push_back("Why did the programmer quit his job? He didn't get arrays!");
    _jokes.push_back("How do you comfort a JavaScript bug? You console it!");
    _jokes.push_back("Why do programmers prefer dark mode? Because light attracts bugs!");
    _jokes.push_back("What do you call a programmer from Finland? Nerdic!");
}

bool Bot::connect() {
    _socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_socket < 0) {
        std::cerr << "Error: Failed to create socket" << std::endl;
        return false;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(_port);
    
    if (inet_pton(AF_INET, _server.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Error: Invalid server address" << std::endl;
        close(_socket);
        return false;
    }

    if (::connect(_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: Failed to connect to server" << std::endl;
        close(_socket);
        return false;
    }

    _connected = true;
    std::cout << "Connected to server " << _server << ":" << _port << std::endl;
    return true;
}

void Bot::authenticate() {
    if (!_connected) return;
    
    // Send PASS command
    sendMessage("PASS " + _password);
    usleep(100000);
    // Send NICK command
    sendMessage("NICK " + _nickname);
    usleep(100000);
    // Send USER command
    sendMessage("USER " + _username + " 0 * : " + _realname);
    
    std::cout << "Authentication sent" << std::endl;
}

void Bot::sendMessage(const std::string& message) {
    if (!_connected) return;
    
    std::string fullMessage = message + "\r\n";
    ssize_t sent = send(_socket, fullMessage.c_str(), fullMessage.length(), 0);
    if (sent < 0) {
        std::cerr << "ERROR: Failed to send message: " << strerror(errno) << std::endl;
        return;
    }
    if (sent != (ssize_t)fullMessage.length()) {
        std::cerr << "WARNING: Partial send - sent " << sent << " of " << fullMessage.length() << " bytes" << std::endl;
    }
    std::cout << "SENT: " << message << std::endl;
}

void Bot::run() {
    if (!_connected) return;
    
    char buffer[1024];
    std::string messageBuffer;
    
    while (_connected) {
        struct pollfd pfd;
        pfd.fd = _socket;
        pfd.events = POLLIN;
        
        int pollResult = poll(&pfd, 1, 1000); // 1 second timeout
        if (pollResult < 0) {
            std::cerr << "Error: Poll failed" << std::endl;
            break;
        }
        
        if (pollResult == 0) continue; // Timeout, continue polling
        
        if (pfd.revents & POLLIN) {
            ssize_t bytesReceived = recv(_socket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytesReceived <= 0) {
                std::cout << "Server disconnected" << std::endl;
                break;
            }
            
            buffer[bytesReceived] = '\0';
            messageBuffer += buffer;
            
            size_t pos;
            while ((pos = messageBuffer.find("\r\n")) != std::string::npos) {
                std::string message = messageBuffer.substr(0, pos);
                messageBuffer.erase(0, pos + 2);
                
                if (!message.empty()) {
                    std::cout << "RECV: " << message << std::endl;
                    handleMessage(message);
                }
            }
        }
        
        if (pfd.revents & (POLLHUP | POLLERR)) {
            std::cout << "Connection error detected" << std::endl;
            break;
        }
    }
}

void Bot::handleMessage(const std::string& message) {
    std::vector<std::string> parts = split(message, ' ');
    if (parts.empty()) return;
    
    // Handle PING messages
    if (parts[0] == "PING") {
        if (parts.size() > 1) {
            sendMessage("PONG " + parts[1]);
        }
        return;
    }
    
    // Handle server responses - check both possible formats
    // Format 1: "001 -> nickname :Welcome..."
    // Format 2: ":server 001 nickname :Welcome..."
    bool isWelcome = false;
    if (parts.size() >= 2 && parts[0] == "001") {
        isWelcome = true;
    } else if (parts.size() >= 3 && parts[1] == "001") {
        isWelcome = true;
    }
    
    if (isWelcome) {
        // Welcome message - we're authenticated
        _authenticated = true;
        std::cout << "Successfully authenticated!" << std::endl;
        joinChannel("#bot");
        return;
    }
    
    // Handle PRIVMSG
    if (parts.size() >= 4 && parts[1] == "PRIVMSG") {
        std::string sender = parts[0].substr(1); // Remove ':' prefix
        size_t exclamPos = sender.find('!');
        if (exclamPos != std::string::npos) {
            sender = sender.substr(0, exclamPos); // Extract nickname
        }
        
        std::string target = parts[2];
        std::string messageText = "";
        
        // Reconstruct the message text from parts[3] onwards
        for (size_t i = 3; i < parts.size(); ++i) {
            if (i == 3) {
                // First part might be just ":" or ":message"
                if (parts[i] == ":") {
                    // Colon is separate, continue to next part
                    continue;
                } else if (parts[i].length() > 0 && parts[i][0] == ':') {
                    // Colon is attached to message
                    messageText += parts[i].substr(1);
                } else {
                    messageText += parts[i];
                }
            } else {
                if (!messageText.empty()) {
                    messageText += " ";
                }
                messageText += parts[i];
            }
        }
        
        handlePrivMsg(sender, target, messageText);
    }
}

void Bot::handlePrivMsg(const std::string& sender, const std::string& target, const std::string& message) {
    std::string responseTarget = (target == _nickname) ? sender : target;
    
    // Check if message starts with bot command
    if (!message.empty() && message[0] == '!') {
        std::istringstream iss(message.substr(1));
        std::string command;
        iss >> command;
        std::string args;
        getline(iss, args);
        if (!args.empty() && args[0] == ' ') {
            args = args.substr(1);
        }
        
        handleCommand(sender, responseTarget, command, args);
    }
}

void Bot::handleCommand(const std::string& sender, const std::string& target, const std::string& command, const std::string& args) {
    (void)args;
    
    if (command == "help") {
        sendPrivMsg(target, "Available commands: !help, !ping, !pong, !hi, !joke");
    }
    else if (command == "ping") {
        sendPrivMsg(target, "Pong!");
    }
    else if (command == "pong") {
        sendPrivMsg(target, "Ping!");
    }
    else if (command == "hi") {
        sendPrivMsg(target, "Hello " + sender + "! How are you doing?");
    }
    else if (command == "joke") {
        sendPrivMsg(target, getRandomJoke());
    }
    else {
        sendPrivMsg(target, "Unknown command: " + command + ". Type !help for available commands.");
    }
}

void Bot::sendPrivMsg(const std::string& target, const std::string& message) {
    sendMessage("PRIVMSG " + target + " :" + message);
}

void Bot::joinChannel(const std::string& channel) {
    sendMessage("JOIN " + channel);
    std::cout << "Joining channel: " << channel << std::endl;
}

std::string Bot::getRandomJoke() {
    if (_jokes.empty()) return "I'm out of jokes!";
    return _jokes[rand() % _jokes.size()];
}

std::vector<std::string> Bot::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

void Bot::disconnect() {
    if (_connected) {
        sendMessage("QUIT :Bot shutting down");
        close(_socket);
        _connected = false;
        _authenticated = false;
        std::cout << "Disconnected from server" << std::endl;
    }
} 
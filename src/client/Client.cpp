#include "../include/Client.hpp"

// Constructor for server-side client
Client::Client(int fd) : sockfd(fd), connected(true) {
    nickname = "";
    username = "";
    realname = "";
}

// Constructor for client-side connection
Client::Client(const std::string& server, int port) 
    : server(server), port(port), connected(false) {
    nickname = "";
    username = "";
    realname = "";
    createSocket();
}

Client::~Client() {
    if (connected) {
        disconnect();
    }
}

bool Client::createSocket() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error: Failed to create socket" << std::endl;
        return false;
    }
    return true;
}

bool Client::connectToServer() {
    struct sockaddr_in server_addr;
    struct hostent* host_entry;

    // Get host by name
    host_entry = gethostbyname(server.c_str());
    if (!host_entry) {
        std::cerr << "Error: Failed to resolve hostname" << std::endl;
        return false;
    }

    // Setup server address structure
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    std::memcpy(&server_addr.sin_addr.s_addr, host_entry->h_addr, host_entry->h_length);

    // Connect to server
    if (::connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error: Failed to connect to server" << std::endl;
        return false;
    }

    return true;
}

bool Client::connect() {
    if (!createSocket()) {
        return false;
    }

    if (!connectToServer()) {
        close(sockfd);
        return false;
    }

    connected = true;
    std::cout << "Connected to " << server << ":" << port << std::endl;
    return true;
}

void Client::disconnect() {
    if (connected) {
        quit("Client disconnecting");
        close(sockfd);
        connected = false;
        std::cout << "Disconnected from server" << std::endl;
    }
}

bool Client::isConnected() const {
    return connected;
}

void Client::setNickname(const std::string& nick) {
    nickname = nick;
}

void Client::setUsername(const std::string& user) {
    username = user;
}

void Client::setRealname(const std::string& real) {
    realname = real;
}

void Client::authenticate() {
    sendRaw("NICK " + nickname);
    sendRaw("USER " + username + " 0 * :" + realname);
}

void Client::sendRaw(const std::string& message) {
    if (!connected) {
        std::cerr << "Error: Not connected to server" << std::endl;
        return;
    }

    std::string msg = message + "\r\n";
    ssize_t sent = send(sockfd, msg.c_str(), msg.length(), 0);
    
    if (sent < 0) {
        std::cerr << "Error: Failed to send message" << std::endl;
    } else {
        std::cout << ">> " << message << std::endl;
    }
}

std::string Client::receiveMessage() {
    if (!connected) {
        return "";
    }

    char buffer[4096];
    ssize_t received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    
    if (received <= 0) {
        if (received == 0) {
            std::cout << "Server closed connection" << std::endl;
        } else {
            std::cerr << "Error receiving message" << std::endl;
        }
        connected = false;
        return "";
    }

    buffer[received] = '\0';
    return std::string(buffer);
}

void Client::processMessages() {
    std::string buffer = receiveMessage();
    if (buffer.empty()) {
        return;
    }

    // Split messages by \r\n
    std::istringstream iss(buffer);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (!line.empty()) {
            parseMessage(line);
        }
    }
}

void Client::parseMessage(const std::string& message) {
    displayMessage(message);
    
    // Handle PING messages
    if (message.substr(0, 4) == "PING") {
        size_t pos = message.find(':');
        if (pos != std::string::npos) {
            std::string server_name = message.substr(pos + 1);
            pong(server_name);
        }
    }
}

std::vector<std::string> Client::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

void Client::join(const std::string& channel) {
    sendRaw("JOIN " + channel);
}

void Client::part(const std::string& channel) {
    sendRaw("PART " + channel);
}

void Client::privmsg(const std::string& target, const std::string& message) {
    sendRaw("PRIVMSG " + target + " :" + message);
}

void Client::quit(const std::string& message) {
    sendRaw("QUIT :" + message);
}

void Client::pong(const std::string& server) {
    sendRaw("PONG :" + server);
}

void Client::handlePing(const std::string& server) {
    pong(server);
}

void Client::displayMessage(const std::string& message) {
    std::cout << "<< " << message << std::endl;
}
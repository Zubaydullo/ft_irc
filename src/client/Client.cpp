#include "../../include/Client.hpp"

// Constructor for server-side client
Client::Client(int fd) : sockfd(fd), connected(true) {
    nickname = "";
    username = "";
    realname = "";
    authenticated = false;
    registered = false;
}

// Constructor for client-side connection
Client::Client(const std::string& server, int port) 
    : server(server), port(port), connected(false) {
    nickname = "";
    username = "";
    realname = "";
    authenticated = false;
    registered = false;
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

std::string Client::getClientIP() const {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    if (getpeername(sockfd, (struct sockaddr*)&addr, &addr_len) == 0) {
        return std::string(inet_ntoa(addr.sin_addr));
    }
    return "127.0.0.1";  
}

void Client::setClientIP(const std::string& ip) {
    clientIP = ip;
}

void Client::addToOutBuffer(const std::string& msg) {
    _outBuffer += msg;
}

std::string& Client::getOutBuffer() {
    return _outBuffer;
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
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        
        if (!line.empty()) {
            parseMessage(line);
        }
    }
}

void Client::parseMessage(const std::string& message) {
    std::cout << "<< " << message << std::endl;
    
    // Handle PING messages
    if (message.substr(0, 4) == "PING") {
        size_t pos = message.find(':');
        if (pos != std::string::npos) {
            std::string server_name = message.substr(pos + 1);
            sendRaw("PONG :" + server_name);
        }
    }
}

// Getter and setter implementations
int Client::getFd() const {
    return sockfd;
}

bool Client::isAuthenticated() const {
    return authenticated;
}

bool Client::isRegistered() const {
    return registered;
}

void Client::setAuthenticated(bool auth) {
    authenticated = auth;
}

void Client::setRegistered(bool reg) {
    registered = reg;
}

std::string Client::getNickname() {
    return nickname;
}

std::string Client::getUsername() {
    return username;
}

std::string Client::getRealname() {
    return realname;
}

void Client::addToInBuffer(const std::string& data) {
    _inBuffer += data;
}

std::string& Client::getInBuffer() {
    return _inBuffer;
}

#include "../../include/Client.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <iostream>
#include <sstream>
#include <cstring>

// Constructor for server-side client
Client::Client(int fd) 
    : _fd(fd), _port(0), _authenticated(false), _registered(false), _lastActivity(std::time(nullptr)) {
    _hostname = "localhost";
    _servername = "42IRC";
    _pingSent = false;
    _pingTime = 0;
}

// Constructor for client-side connection
Client::Client(const std::string& server, int port) 
    : _fd(-1), _server(server), _port(port), _authenticated(false), _registered(false), _lastActivity(std::time(nullptr)) {
    _hostname = "localhost";
    _servername = "42IRC";
    createSocket();
    _pingSent = false;
    _pingTime = 0;
}

Client::~Client() {
        disconnect();
}

bool Client::createSocket() {
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0) {
        handleError("Failed to create socket");
        return false;
    }
    return true;
}

bool Client::connectToServer(const std::string& host, int port) {
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(host.c_str());

    if (::connect(_fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        handleError("Failed to connect to server");
        return false;
    }

    return true;
}

bool Client::connect() {
    if (!createSocket()) {
        return false;
    }

    if (!connectToServer(_server, _port)) {
        close(_fd);
        _fd = -1;
        return false;
    }

    _authenticated = true;
    _registered = true;
    updateActivity();
    return true;
}

void Client::disconnect() {
    if (_fd != -1) {
        quit("Client disconnecting");
        close(_fd);
        _fd = -1;
        handleStateChange("Disconnected");
    }
}

bool Client::isConnected() const {
    return _fd != -1;
}

void Client::updateActivity() {
    _lastActivity = std::time(nullptr);
}

bool Client::sendRaw(const std::string& message) {
    if (_fd == -1) {
        return false;
    }

    std::string msg = message + "\r\n";
    if (send(_fd, msg.c_str(), msg.length(), 0) == -1) {
        handleError("Failed to send message");
        return false;
    }

    return true;
}

bool Client::sendMessage(const std::string& target, const std::string& message) {
    if (!isRegistered()) {
        handleError("Not registered");
        return false;
    }

    return sendRaw("PRIVMSG " + target + " :" + message);
}

bool Client::sendNotice(const std::string& target, const std::string& message) {
    if (!isRegistered()) {
        handleError("Not registered");
        return false;
    }

    return sendRaw("NOTICE " + target + " :" + message);
}

bool Client::joinChannel(const std::string& channel) {
    if (!isRegistered()) {
        handleError("Not registered");
        return false;
    }

    if (!validateChannelName(channel)) {
        handleError("Invalid channel name");
        return false;
    }

    if (sendRaw("JOIN " + channel)) {
        _channels.insert(channel);
        return true;
    }

    return false;
}

bool Client::partChannel(const std::string& channel) {
    if (!isRegistered()) {
        handleError("Not registered");
        return false;
    }

    if (!validateChannelName(channel)) {
        handleError("Invalid channel name");
        return false;
    }

    if (sendRaw("PART " + channel)) {
        _channels.erase(channel);
        return true;
    }

    return false;
}

bool Client::setNickname(const std::string& nickname) {
    if (!validateNickname(nickname)) {
        handleError("Invalid nickname");
        return false;
    }

    if (sendRaw("NICK " + nickname)) {
        _nickname = nickname;
        updateActivity();
        return true;
    }

    return false;
}

bool Client::setUsername(const std::string& username) {
    if (username.empty() || username.length() > 12) {
        handleError("Invalid username");
        return false;
    }

    _username = username;
    updateActivity();
    return true;
}

bool Client::setRealname(const std::string& realname) {
    _realname = realname;
    updateActivity();
    return true;
    }

bool Client::setHostname(const std::string& hostname) {
    _hostname = hostname;
    updateActivity();
    return true;
}

bool Client::setServername(const std::string& servername) {
    _servername = servername;
    updateActivity();
    return true;
}

bool Client::setChannelMode(const std::string& channel, const std::string& mode) {
    if (!isRegistered()) {
        handleError("Not registered");
        return false;
    }

    return sendRaw("MODE " + channel + " " + mode);
}

bool Client::setTopic(const std::string& channel, const std::string& topic) {
    if (!isRegistered()) {
        handleError("Not registered");
        return false;
    }

    return sendRaw("TOPIC " + channel + " :" + topic);
        }

bool Client::kickUser(const std::string& channel, const std::string& nickname, const std::string& reason) {
    if (!isRegistered()) {
        handleError("Not registered");
        return false;
    }

    return sendRaw("KICK " + channel + " " + nickname + " :" + reason);
}

bool Client::inviteUser(const std::string& channel, const std::string& nickname) {
    if (!isRegistered()) {
        handleError("Not registered");
        return false;
    }

    return sendRaw("INVITE " + nickname + " " + channel);
        }

bool Client::quit(const std::string& reason) {
    if (sendRaw("QUIT :" + reason)) {
        disconnect();
        return true;
    }

    return false;
}

bool Client::ping(const std::string& server) {
    return sendRaw("PING " + server);
}

bool Client::pong(const std::string& server) {
    return sendRaw("PONG " + server);
}

bool Client::who(const std::string& target) {
    if (!isRegistered()) {
        handleError("Not registered");
        return false;
    }

    return sendRaw("WHO " + target);
}

bool Client::list(const std::string& channel) {
    if (!isRegistered()) {
        handleError("Not registered");
        return false;
    }
    
    return sendRaw("LIST " + channel);
}

bool Client::validateNickname(const std::string& nickname) {
    if (nickname.empty() || nickname.length() > 9) {
        return false;
    }

    for (size_t i = 0; i < nickname.length(); ++i) {
        char c = nickname[i];
        if (!isalnum(c) && c != '-' && c != '_' && c != '[' && c != ']' && 
            c != '\\' && c != '`' && c != '^' && c != '{' && c != '}') {
            return false;
}
    }

    return true;
}

bool Client::validateChannelName(const std::string& channel) {
    if (channel.empty() || channel.length() > 50) {
        return false;
    }

    if (channel[0] != '#' && channel[0] != '&') {
        return false;
    }

    for (size_t i = 1; i < channel.length(); i++) {
        if (channel[i] == ' ' || channel[i] == ',' || channel[i] == 7) {
            return false;
        }
    }

    return true;
}

void Client::handleError(const std::string& error) {
    std::cerr << "Client Error: " << error << std::endl;
}

void Client::handleStateChange(const std::string& state) {
    std::cout << "State changed: " << state << std::endl;
}

void Client::handleConnectionError() {
    std::cout << "Server closed connection" << std::endl;
    _fd = -1;
}

// Getters
int Client::getFd() const { return _fd; }
std::string Client::getNickname() const { return _nickname; }
std::string Client::getUsername() const { return _username; }
std::string Client::getRealname() const { return _realname; }
std::string Client::getHostname() const { return _hostname; }
std::string Client::getServername() const { return _servername; }
std::time_t Client::getLastActivity() const { return _lastActivity; }
bool Client::isAuthenticated() const { return _authenticated; }
bool Client::isRegistered() const { return _registered; }
bool Client::isPingSent() const {
    return _pingSent;
}
std::time_t Client::getPingTime() const {
    return _pingTime;
}

// Setters
void Client::setAuthenticated(bool authenticated) {
    _authenticated = authenticated;
}

void Client::setRegistered(bool registered) {
    _registered = registered;
}

void Client::setPingSent(bool pingSent) {
    _pingSent = pingSent;
}

void Client::setPingTime(std::time_t pingTime) {
    _pingTime = pingTime;
}

// Channel management
std::set<std::string> Client::getChannels() const {
    return _channels;
}

bool Client::isInChannel(const std::string& channel) const {
    std::set<std::string>::const_iterator it = _channels.find(channel);
    return it != _channels.end();
}

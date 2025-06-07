#include "../../include/Server.hpp"
#include "../../include/Client.hpp"
#include <ctime>
#include <sstream>
#include <algorithm>
#include <fcntl.h>

// Forward declarations
void sendWelcomeMessages(int clientFd);
std::string getChannelMembers(const std::string& channel);

Server::Server(int port, const std::string& password)
    : _port(port), _password(password), _serverSocket(-1), _running(false) {
    _serverName = "42IRC";
    _serverVersion = "1.0";
    _serverCreationDate = std::to_string(std::time(nullptr));
    _maxClients = 100;
    _maxChannels = 50;
    _startTime = std::time(nullptr);
}

Server::~Server() {
    stop();
    std::map<int, Client*>::iterator clientIt;
    for (clientIt = _clients.begin(); clientIt != _clients.end(); ++clientIt) {
        delete clientIt->second;
    }
    std::map<std::string, Channel*>::iterator channelIt;
    for (channelIt = _channels.begin(); channelIt != _channels.end(); ++channelIt) {
        delete channelIt->second;
    }
}

bool Server::createSocket() {
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket == -1) {
        handleError("Failed to create socket");
        return false;
    }

    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        handleError("Failed to set socket options");
        return false;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(_port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        handleError("Failed to bind socket to port " + std::to_string(_port));
        close(_serverSocket);
           return false;
       }

    if (listen(_serverSocket, 10) == -1) {
        handleError("Failed to listen on port " + std::to_string(_port));
        close(_serverSocket);
        return false;
     }

    return true;
}

void Server::start() {
    if (!createSocket()) {
        throw std::runtime_error("Failed to create server socket");
    }

    // Set server socket to non-blocking mode
    int flags = fcntl(_serverSocket, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("Failed to get socket flags");
    }
    if (fcntl(_serverSocket, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("Failed to set non-blocking mode");
    }

    struct pollfd serverPoll;
    serverPoll.fd = _serverSocket;
    serverPoll.events = POLLIN;
    serverPoll.revents = 0;
    _pollfd.push_back(serverPoll);
    
    _running = true;
    std::cout << "Server started on port " << _port << std::endl;

    while (_running) {
        int activity = poll(_pollfd.data(), _pollfd.size(), 1000); // 1 second timeout
        if (activity == -1) {
            if (errno == EINTR) {
                continue; // Interrupted by signal, try again
            }
            handleError("Poll error");
                break;
         }

        checkClientTimeout();

        for (size_t i = 0; i < _pollfd.size(); i++) {
            if (!(_pollfd[i].revents & POLLIN)) {
                continue;
            }

            if (_pollfd[i].fd == _serverSocket) {
                 acceptNewClient();
            } else {
                handleClientData(_pollfd[i].fd);
            }
        }
    }
}

void Server::stop() {
    _running = false;
    if (_serverSocket != -1) {
        close(_serverSocket);
        _serverSocket = -1;
    }
}

void Server::acceptNewClient() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientFd = accept(_serverSocket, (struct sockaddr*)&clientAddr, &clientLen);

    if (clientFd == -1) {
        handleError("Failed to accept new client");
        return;
    }

    if (_clients.size() >= _maxClients) {
        sendToClient(clientFd, "ERROR :Server is full");
        close(clientFd);
        return;
    }

    // Set non-blocking mode
    int flags = fcntl(clientFd, F_GETFL, 0);
    if (flags == -1) {
        handleError("Failed to get socket flags");
        close(clientFd);
        return;
    }
    if (fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1) {
        handleError("Failed to set non-blocking mode");
        close(clientFd);
        return;
    }

    // Add to pollfd
    struct pollfd clientPoll;
    clientPoll.fd = clientFd;
    clientPoll.events = POLLIN;
    clientPoll.revents = 0;
    _pollfd.push_back(clientPoll);

    // Create and store client
    Client* newClient = new Client(clientFd);
    if (!newClient) {
        handleError("Failed to create new client");
        close(clientFd);
        return;
    }

    _clients[clientFd] = newClient;
    std::cout << "New client connected: " << clientFd << std::endl;
}

void Server::handleClientData(int clientFd) {
    if (_clients.find(clientFd) == _clients.end() || !_clients[clientFd]) {
        std::cout << "Client " << clientFd << " not found in handleClientData" << std::endl;
        return;
    }

    char buffer[1024];
    int bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
         
    if (bytesRead <= 0) {
        if (bytesRead == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return; // No data available, try again later
        }
        // Client disconnected
        std::cout << "Client " << clientFd << " disconnected (recv returned " << bytesRead << ")" << std::endl;
        close(clientFd);
        removeClient(clientFd);
        return;
    }

    buffer[bytesRead] = '\0';
    std::string message(buffer);
    
    // Update client activity
    _clients[clientFd]->updateActivity();
    
    // Split message into lines in case multiple commands are received
    std::istringstream iss(message);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        if (!line.empty()) {
            try {
                parseCommand(clientFd, line);
            } catch (const std::exception& e) {
                std::cout << "Error parsing command: " << e.what() << std::endl;
                sendToClient(clientFd, "500 :Internal server error");
            }
        }
    }
}

void Server::removeClient(int clientFd) {
    static std::set<int> removingClients; // Track clients being removed to prevent recursion
    
    if (removingClients.find(clientFd) != removingClients.end()) {
        std::cout << "Client " << clientFd << " is already being removed" << std::endl;
        return;
    }
    
    removingClients.insert(clientFd);
    std::cout << "Removing client " << clientFd << std::endl;
    
    if (_clients.find(clientFd) == _clients.end()) {
        std::cout << "Client not found in removeClient" << std::endl;
        removingClients.erase(clientFd);
        return;
    }

    Client* client = _clients[clientFd];
    if (!client) {
        std::cout << "Client pointer is null in removeClient" << std::endl;
        removingClients.erase(clientFd);
        return;
    }

    // Store nickname before removing client
    std::string nickname = client->getNickname();
    std::cout << "Removing client with nickname: " << (nickname.empty() ? "none" : nickname) << std::endl;

    // Remove from channels first
    std::vector<std::string> channelsToRemove;
    std::map<std::string, Channel*>::iterator channelIt;
    for (channelIt = _channels.begin(); channelIt != _channels.end(); ++channelIt) {
        if (channelIt->second && channelIt->second->hasMember(clientFd)) {
            channelIt->second->removeMember(clientFd);
            if (channelIt->second->getMemberCount() == 0) {
                channelsToRemove.push_back(channelIt->first);
            }
        }
    }

    // Remove empty channels
    for (size_t i = 0; i < channelsToRemove.size(); ++i) {
        if (_channels.find(channelsToRemove[i]) != _channels.end()) {
            delete _channels[channelsToRemove[i]];
            _channels.erase(channelsToRemove[i]);
        }
    }

    // Remove from nickname map
    if (!nickname.empty()) {
        _nicknameMap.erase(nickname);
    }

    // Remove from pollfd
    for (size_t i = 0; i < _pollfd.size(); ++i) {
        if (_pollfd[i].fd == clientFd) {
            _pollfd.erase(_pollfd.begin() + i);
            break;
        }
    }

    // Clean up client
    delete client;
    _clients.erase(clientFd);
    
    removingClients.erase(clientFd);
    std::cout << "Client " << clientFd << " removed successfully" << std::endl;
}

void Server::parseCommand(int clientFd, const std::string& message) {
    if (_clients.find(clientFd) == _clients.end() || !_clients[clientFd]) {
        std::cout << "Client " << clientFd << " not found in parseCommand" << std::endl;
        return;
    }

    std::istringstream iss(message);
    std::string command;
    iss >> command;

    // Convert command to uppercase
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);

    try {
        if (command == "PASS") {
            handlePass(clientFd, iss);
        } else if (command == "NICK") {
            handleNick(clientFd, iss);
        } else if (command == "USER") {
            handleUser(clientFd, iss);
        } else if (command == "JOIN") {
            handleJoin(clientFd, iss);
        } else if (command == "PART") {
            handlePart(clientFd, iss);
        } else if (command == "PRIVMSG") {
            handlePrivmsg(clientFd, iss);
        } else if (command == "NOTICE") {
            handleNotice(clientFd, iss);
        } else if (command == "MODE") {
            handleMode(clientFd, iss);
        } else if (command == "TOPIC") {
            handleTopic(clientFd, iss);
        } else if (command == "KICK") {
            handleKick(clientFd, iss);
        } else if (command == "INVITE") {
            handleInvite(clientFd, iss);
        } else if (command == "QUIT") {
            handleQuit(clientFd, iss);
        } else if (command == "PING") {
            handlePing(clientFd, iss);
        } else if (command == "PONG") {
            handlePong(clientFd, iss);
        } else if (command == "WHO") {
            handleWho(clientFd, iss);
        } else if (command == "LIST") {
            handleList(clientFd, iss);
        } else if (!command.empty()) {
            sendToClient(clientFd, "421 " + command + " :Unknown command");
        }
    } catch (const std::exception& e) {
        std::cout << "Error handling command " << command << ": " << e.what() << std::endl;
        sendToClient(clientFd, "500 :Internal server error");
    }
}

void Server::handlePass(int clientFd, std::istringstream& iss) {
    std::cout << "Handling PASS command for client " << clientFd << std::endl;

    // First check if client exists
    if (_clients.find(clientFd) == _clients.end()) {
        std::cout << "Client not found in handlePass" << std::endl;
        return;
    }

    Client* client = _clients[clientFd];
    if (!client) {
        std::cout << "Client pointer is null in handlePass" << std::endl;
        return;
    }

    // If client is already authenticated, ignore PASS command
    if (client->isAuthenticated()) {
        std::cout << "Client already authenticated" << std::endl;
        return;
    }

    // Get password from input
    std::string password;
    iss >> password;

    // Remove any leading/trailing whitespace
    password.erase(0, password.find_first_not_of(" \t\r\n"));
    password.erase(password.find_last_not_of(" \t\r\n") + 1);

    // Check if password is empty
    if (password.empty()) {
        std::cout << "Empty password received" << std::endl;
        sendToClient(clientFd, "461 PASS :Not enough parameters");
        return;
    }

    // Check if password matches
    if (password != _password) {
        std::cout << "Password incorrect: received '" << password << "', expected '" << _password << "'" << std::endl;
        sendToClient(clientFd, "464 :Password incorrect");
        return;  // Just return, allowing client to try again
    }

    // Set client as authenticated (password is correct)
    client->setAuthenticated(true);
    std::cout << "Client password authenticated successfully" << std::endl;

    // Check if client can now be fully registered (has NICK and USER)
    if (!client->getNickname().empty() && !client->getUsername().empty()) {
        client->setRegistered(true);
        sendWelcomeMessages(clientFd);
        std::cout << "Client " << clientFd << " fully registered" << std::endl;
    }
}

void Server::handleNick(int clientFd, std::istringstream& iss) {
    if (_clients.find(clientFd) == _clients.end()) {
        return;
    }

    Client* client = _clients[clientFd];
    if (!client) {
        return;
    }

    // Update activity
    client->updateActivity();

    std::string nickname;
    iss >> nickname;

    // Remove any leading/trailing whitespace
    nickname.erase(0, nickname.find_first_not_of(" \t\r\n"));
    nickname.erase(nickname.find_last_not_of(" \t\r\n") + 1);

    if (nickname.empty()) {
        sendToClient(clientFd, "431 :No nickname given");
        return;
    }

    if (!validateNickname(nickname)) {
        sendToClient(clientFd, "432 " + nickname + " :Erroneous nickname");
        return;
    }

    if (_nicknameMap.find(nickname) != _nicknameMap.end()) {
        sendToClient(clientFd, "433 " + nickname + " :Nickname is already in use");
        return;
    }

    std::string oldNick = client->getNickname();
    if (!oldNick.empty()) {
        _nicknameMap.erase(oldNick);
    }

    client->setNickname(nickname);
    _nicknameMap[nickname] = client;
    std::cout << "Client " << clientFd << " set nickname to: " << nickname << std::endl;

    // Check if client can now be fully registered (has PASS, NICK, and USER)
    if (client->isAuthenticated() && !client->getUsername().empty() && !client->isRegistered()) {
        client->setRegistered(true);
        sendWelcomeMessages(clientFd);
        std::cout << "Client " << clientFd << " fully registered" << std::endl;
    }
}

void Server::handleUser(int clientFd, std::istringstream& iss) {
    if (_clients.find(clientFd) == _clients.end()) {
        return;
    }

    Client* client = _clients[clientFd];
    if (!client) {
        return;
    }

    // Update activity
    client->updateActivity();

    // If client is already registered, ignore USER command
    if (client->isRegistered()) {
        std::cout << "Client already registered, ignoring USER command" << std::endl;
        return;
    }

    std::string username, hostname, servername, realname;
    iss >> username >> hostname >> servername;
    std::getline(iss, realname);

    // Remove any leading/trailing whitespace
    username.erase(0, username.find_first_not_of(" \t\r\n"));
    username.erase(username.find_last_not_of(" \t\r\n") + 1);
    hostname.erase(0, hostname.find_first_not_of(" \t\r\n"));
    hostname.erase(hostname.find_last_not_of(" \t\r\n") + 1);
    servername.erase(0, servername.find_first_not_of(" \t\r\n"));
    servername.erase(servername.find_last_not_of(" \t\r\n") + 1);
    realname.erase(0, realname.find_first_not_of(" \t\r\n"));
    realname.erase(realname.find_last_not_of(" \t\r\n") + 1);

    if (username.empty() || hostname.empty() || servername.empty()) {
        sendToClient(clientFd, "461 USER :Not enough parameters");
        return;
    }

    if (!realname.empty() && realname[0] == ':') {
        realname = realname.substr(1);
    }

    client->setUsername(username);
    client->setHostname(hostname);
    client->setServername(servername);
    client->setRealname(realname);
    std::cout << "Client " << clientFd << " set user info: " << username << std::endl;

    // Check if client can now be fully registered (has PASS, NICK, and USER)
    if (client->isAuthenticated() && !client->getNickname().empty() && !client->isRegistered()) {
        client->setRegistered(true);
        sendWelcomeMessages(clientFd);
        std::cout << "Client " << clientFd << " fully registered" << std::endl;
    }
}

void Server::handleJoin(int clientFd, std::istringstream& iss) {
    if (_clients.find(clientFd) == _clients.end()) {
        return;
    }

    Client* client = _clients[clientFd];
    if (!client) {
        return;
    }

    // Check if client is fully registered (PASS + NICK + USER)
    if (!client->isRegistered()) {
        sendToClient(clientFd, "451 :You have not registered");
        return;
    }
      
    std::string channelName;
    iss >> channelName;

    if (!validateChannelName(channelName)) {
        sendToClient(clientFd, "403 " + channelName + " :No such channel");
        return;
    }
         
    if (_channels.size() >= _maxChannels) {
        sendToClient(clientFd, "405 " + channelName + " :You have joined too many channels");
        return;
    }

    // Check if channel exists
    bool channelExists = (_channels.find(channelName) != _channels.end());
    
    if (!channelExists) {
        // Create new channel and make the creator an operator
        _channels[channelName] = new Channel(channelName);
        Channel* channel = _channels[channelName];
        channel->addMember(clientFd);
        channel->addOperator(clientFd);  // Make creator an operator
        client->joinChannel(channelName);
        
        std::cout << "Client " << clientFd << " created and joined channel " << channelName << " as operator" << std::endl;
    } else {
        // Join existing channel
        Channel* channel = _channels[channelName];
        channel->addMember(clientFd);
        client->joinChannel(channelName);
        
        std::cout << "Client " << clientFd << " joined existing channel " << channelName << std::endl;
    }

    // Send join messages
    std::string nick = client->getNickname();
    broadcastToChannel(channelName, ":" + nick + " JOIN " + channelName);
    sendToClient(clientFd, "353 " + nick + " = " + channelName + " :" + getChannelMembers(channelName));
    sendToClient(clientFd, "366 " + nick + " " + channelName + " :End of /NAMES list");
}

void Server::handlePart(int clientFd, std::istringstream& iss) {
    if (_clients.find(clientFd) == _clients.end()) {
        return;
    }

    Client* client = _clients[clientFd];
    if (!client) {
        return;
    }

    // Check if client is fully registered (PASS + NICK + USER)
    if (!client->isRegistered()) {
        sendToClient(clientFd, "451 :You have not registered");
        return;
    }

    std::string channelName;
    iss >> channelName;

    if (_channels.find(channelName) == _channels.end()) {
        sendToClient(clientFd, "403 " + channelName + " :No such channel");
        return;
    }

    Channel* channel = _channels[channelName];
    if (!channel->hasMember(clientFd)) {
        sendToClient(clientFd, "442 " + channelName + " :You're not on that channel");
        return;
    }

    std::string nick = client->getNickname();
    broadcastToChannel(channelName, ":" + nick + " PART " + channelName);
    channel->removeMember(clientFd);
    client->partChannel(channelName);

    if (channel->getMemberCount() == 0) {
        delete channel;
        _channels.erase(channelName);
    }
}

void Server::handlePrivmsg(int clientFd, std::istringstream& iss) {
    if (_clients.find(clientFd) == _clients.end()) {
        return;
    }

    Client* client = _clients[clientFd];
    if (!client) {
        return;
    }

    // Check if client is fully registered (PASS + NICK + USER)
    if (!client->isRegistered()) {
        sendToClient(clientFd, "451 :You have not registered");
        return;
    }

    // Update activity
    client->updateActivity();

    std::string target, message;
    iss >> target;
    std::getline(iss, message);
    if (!message.empty() && message[0] == ' ') {
        message = message.substr(1);
    }
    if (!message.empty() && message[0] == ':') {
        message = message.substr(1);
    }

    std::string senderNick = client->getNickname();

    if (target[0] == '#') {
        if (_channels.find(target) == _channels.end()) {
            sendToClient(clientFd, "403 " + target + " :No such channel");
            return;
        }

        Channel* channel = _channels[target];
        if (!channel->hasMember(clientFd)) {
            sendToClient(clientFd, "404 " + target + " :Cannot send to channel");
            return;
        }

        broadcastToChannel(target, ":" + senderNick + " PRIVMSG " + target + " :" + message, clientFd);
    } else {
        Client* targetClient = getClientByNickname(target);
        if (!targetClient) {
            sendToClient(clientFd, "401 " + target + " :No such nick");
            return;
        }

        sendToClient(targetClient->getFd(), ":" + senderNick + " PRIVMSG " + target + " :" + message);
    }
}

void Server::handleNotice(int clientFd, std::istringstream& iss) {
    // Similar to PRIVMSG but no error responses
    std::string target, message;
    iss >> target;
    std::getline(iss, message);
    if (!message.empty() && message[0] == ' ') {
        message = message.substr(1);
    }
    if (!message.empty() && message[0] == ':') {
        message = message.substr(1);
    }

    Client* sender = _clients[clientFd];
    std::string senderNick = sender->getNickname();

    if (target[0] == '#') {
        if (_channels.find(target) != _channels.end()) {
            Channel* channel = _channels[target];
            if (channel->hasMember(clientFd)) {
                broadcastToChannel(target, ":" + senderNick + " NOTICE " + target + " :" + message, clientFd);
            }
        }
    } else {
        Client* targetClient = getClientByNickname(target);
        if (targetClient) {
            sendToClient(targetClient->getFd(), ":" + senderNick + " NOTICE " + target + " :" + message);
        }
    }
}

void Server::handleMode(int clientFd, std::istringstream& iss) {
    std::string target, mode;
    iss >> target >> mode;

    if (target[0] == '#') {
        if (_channels.find(target) == _channels.end()) {
            sendToClient(clientFd, "403 " + target + " :No such channel");
            return;
        }

        Channel* channel = _channels[target];
        if (!channel->isOperator(clientFd)) {
            sendToClient(clientFd, "482 " + target + " :You're not channel operator");
            return;
        }

        // Handle channel modes
        channel->setMode(mode);
        broadcastToChannel(target, ":" + _clients[clientFd]->getNickname() + " MODE " + target + " " + mode);
    } else {
        // Handle user modes
        if (target != _clients[clientFd]->getNickname()) {
            sendToClient(clientFd, "502 :Cannot change mode for other users");
            return;
        }

        _clients[clientFd]->setChannelMode(target, mode);
        sendToClient(clientFd, ":" + _clients[clientFd]->getNickname() + " MODE " + target + " " + mode);
    }
}

void Server::handleTopic(int clientFd, std::istringstream& iss) {
    std::string channelName, topic;
    iss >> channelName;
    std::getline(iss, topic);
    if (!topic.empty() && topic[0] == ' ') {
        topic = topic.substr(1);
    }
    if (!topic.empty() && topic[0] == ':') {
        topic = topic.substr(1);
    }

    if (_channels.find(channelName) == _channels.end()) {
        sendToClient(clientFd, "403 " + channelName + " :No such channel");
        return;
    }

    Channel* channel = _channels[channelName];
    if (!channel->hasMember(clientFd)) {
        sendToClient(clientFd, "442 " + channelName + " :You're not on that channel");
        return;
    }

    if (topic.empty()) {
        // Request current topic
        std::string currentTopic = channel->getTopic();
        if (currentTopic.empty()) {
            sendToClient(clientFd, "331 " + channelName + " :No topic is set");
        } else {
            sendToClient(clientFd, "332 " + channelName + " :" + currentTopic);
        }
    } else {
        // Set new topic
        if (!channel->isOperator(clientFd) && channel->isTopicProtected()) {
            sendToClient(clientFd, "482 " + channelName + " :You're not channel operator");
            return;
        }

        channel->setTopic(topic);
        broadcastToChannel(channelName, ":" + _clients[clientFd]->getNickname() + " TOPIC " + channelName + " :" + topic);
    }
}

void Server::handleKick(int clientFd, std::istringstream& iss) {
    std::string channelName, targetNick, reason;
    iss >> channelName >> targetNick;
    std::getline(iss, reason);
    if (!reason.empty() && reason[0] == ' ') {
        reason = reason.substr(1);
    }
    if (!reason.empty() && reason[0] == ':') {
        reason = reason.substr(1);
    }

    if (_channels.find(channelName) == _channels.end()) {
        sendToClient(clientFd, "403 " + channelName + " :No such channel");
        return;
    }

    Channel* channel = _channels[channelName];
    if (!channel->isOperator(clientFd)) {
        sendToClient(clientFd, "482 " + channelName + " :You're not channel operator");
        return;
    }

    Client* targetClient = getClientByNickname(targetNick);
    if (!targetClient || !channel->hasMember(targetClient->getFd())) {
        sendToClient(clientFd, "441 " + targetNick + " " + channelName + " :They aren't on that channel");
        return;
    }

    std::string kickerNick = _clients[clientFd]->getNickname();
    broadcastToChannel(channelName, ":" + kickerNick + " KICK " + channelName + " " + targetNick + " :" + reason);
    channel->removeMember(targetClient->getFd());
    targetClient->partChannel(channelName);

    if (channel->getMemberCount() == 0) {
        delete channel;
        _channels.erase(channelName);
    }
}

void Server::handleInvite(int clientFd, std::istringstream& iss) {
    std::string targetNick, channelName;
    iss >> targetNick >> channelName;

    if (_channels.find(channelName) == _channels.end()) {
        sendToClient(clientFd, "403 " + channelName + " :No such channel");
        return;
    }

    Channel* channel = _channels[channelName];
    if (!channel->isOperator(clientFd)) {
        sendToClient(clientFd, "482 " + channelName + " :You're not channel operator");
        return;
    }

    Client* targetClient = getClientByNickname(targetNick);
    if (!targetClient) {
        sendToClient(clientFd, "401 " + targetNick + " :No such nick");
        return;
    }

    if (channel->hasMember(targetClient->getFd())) {
        sendToClient(clientFd, "443 " + targetNick + " " + channelName + " :is already on channel");
        return;
    }

    std::string inviterNick = _clients[clientFd]->getNickname();
    sendToClient(targetClient->getFd(), ":" + inviterNick + " INVITE " + targetNick + " " + channelName);
    sendToClient(clientFd, "341 " + inviterNick + " " + targetNick + " " + channelName);
}

void Server::handleQuit(int clientFd, std::istringstream& iss) {
    if (_clients.find(clientFd) == _clients.end()) {
        return;
    }

    Client* client = _clients[clientFd];
    if (!client) {
        return;
    }

    std::string reason;
    std::getline(iss, reason);
    if (!reason.empty() && reason[0] == ' ') {
        reason = reason.substr(1);
    }
    if (!reason.empty() && reason[0] == ':') {
        reason = reason.substr(1);
    }

    if (reason.empty()) {
        reason = "Client Quit";
    }

    std::string nick = client->getNickname();
    if (!nick.empty()) {
        std::vector<std::string> channels;
        std::set<std::string> clientChannels = client->getChannels();
        std::set<std::string>::const_iterator it;
        for (it = clientChannels.begin(); it != clientChannels.end(); ++it) {
            channels.push_back(*it);
        }

        for (size_t i = 0; i < channels.size(); ++i) {
            if (_channels.find(channels[i]) != _channels.end() && _channels[channels[i]]) {
                broadcastToChannel(channels[i], ":" + nick + " QUIT :" + reason);
            }
        }
    }

    removeClient(clientFd);
}

void Server::handlePing(int clientFd, std::istringstream& iss) {
    if (_clients.find(clientFd) == _clients.end() || !_clients[clientFd]) {
        return;
    }

    std::string server;
    iss >> server;
    
    // Update client's last activity time
    _clients[clientFd]->updateActivity();
    _clients[clientFd]->setPingSent(false);
    
    sendToClient(clientFd, "PONG " + server);
    std::cout << "Responded to PING from client " << clientFd << std::endl;
}

void Server::handlePong(int clientFd, std::istringstream& iss) {
    if (_clients.find(clientFd) == _clients.end() || !_clients[clientFd]) {
        return;
    }

    std::string server;
    iss >> server;

    // Update client's last activity time and reset ping status
    _clients[clientFd]->updateActivity();
    _clients[clientFd]->setPingSent(false);
    std::cout << "Received PONG from client " << clientFd << std::endl;
}

void Server::handleWho(int clientFd, std::istringstream& iss) {
    std::string target;
    iss >> target;

    if (target[0] == '#') {
        if (_channels.find(target) == _channels.end()) {
            sendToClient(clientFd, "315 " + target + " :End of /WHO list");
            return;
        }

        Channel* channel = _channels[target];
        std::vector<int> members = channel->getMembers();
        for (size_t i = 0; i < members.size(); ++i) {
            int memberFd = members[i];
            Client* member = _clients[memberFd];
            std::string nick = member->getNickname();
            std::string user = member->getUsername();
            std::string host = member->getHostname();
            std::string real = member->getRealname();
            std::string flags = channel->isOperator(memberFd) ? "@" : "";

            sendToClient(clientFd, "352 " + target + " " + user + " " + host + " " + _serverName + " " + nick + " " + flags + " :0 " + real);
        }
        sendToClient(clientFd, "315 " + target + " :End of /WHO list");
    } else {
        Client* targetClient = getClientByNickname(target);
        if (targetClient) {
            std::string nick = targetClient->getNickname();
            std::string user = targetClient->getUsername();
            std::string host = targetClient->getHostname();
            std::string real = targetClient->getRealname();

            sendToClient(clientFd, "352 * " + user + " " + host + " " + _serverName + " " + nick + " H :0 " + real);
        }
        sendToClient(clientFd, "315 " + target + " :End of /WHO list");
    }
}

void Server::handleList(int clientFd, std::istringstream& iss) {
    std::string channelName;
    iss >> channelName;

    if (channelName.empty()) {
        // List all channels
        std::map<std::string, Channel*>::iterator channelIt;
        for (channelIt = _channels.begin(); channelIt != _channels.end(); ++channelIt) {
            std::string name = channelIt->first;
            int memberCount = channelIt->second->getMemberCount();
            std::string topic = channelIt->second->getTopic();
            sendToClient(clientFd, "322 " + name + " " + std::to_string(memberCount) + " :" + topic);
        }
    } else {
        // List specific channel
        if (_channels.find(channelName) != _channels.end()) {
            Channel* channel = _channels[channelName];
            int memberCount = channel->getMemberCount();
            std::string topic = channel->getTopic();
            sendToClient(clientFd, "322 " + channelName + " " + std::to_string(memberCount) + " :" + topic);
        }
    }

    sendToClient(clientFd, "323 :End of /LIST");
}

void Server::sendToClient(int clientFd, const std::string& message) {
    if (_clients.find(clientFd) == _clients.end() || !_clients[clientFd]) {
        std::cout << "Client " << clientFd << " not found in sendToClient" << std::endl;
        return;
    }

    std::string msg = message + "\r\n";
    int bytesSent = send(clientFd, msg.c_str(), msg.length(), MSG_NOSIGNAL);
    if (bytesSent == -1) {
        if (errno != EPIPE && errno != ECONNRESET) {
            std::cout << "Error sending message to client " << clientFd << ": " << strerror(errno) << std::endl;
        }
        // Don't close and remove here, let the main loop handle it
        return;
    }
}

void Server::broadcastToChannel(const std::string& channel, const std::string& message, int excludeFd) {
    if (_channels.find(channel) == _channels.end() || !_channels[channel]) {
        std::cout << "Channel " << channel << " not found in broadcastToChannel" << std::endl;
        return;
    }

    Channel* channelPtr = _channels[channel];
    std::vector<int> members = channelPtr->getMembers();
    for (size_t i = 0; i < members.size(); ++i) {
        int memberFd = members[i];
        if (memberFd != excludeFd && _clients.find(memberFd) != _clients.end() && _clients[memberFd]) {
            sendToClient(memberFd, message);
        }
    }
}

bool Server::validateNickname(const std::string& nickname) {
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

bool Server::validateChannelName(const std::string& channel) {
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

void Server::checkClientTimeout() {
    std::time_t now = std::time(nullptr);
    std::vector<int> timeoutClients;

    std::map<int, Client*>::const_iterator clientIt;
    for (clientIt = _clients.begin(); clientIt != _clients.end(); ++clientIt) {
        if (clientIt->second) {
            std::time_t lastActivity = clientIt->second->getLastActivity();
            std::time_t timeSinceLastActivity = now - lastActivity;
            
            // If no activity for 5 minutes, send PING
            if (timeSinceLastActivity > 300 && !clientIt->second->isPingSent()) {
                std::cout << "Sending PING to client " << clientIt->first << " (inactive for " << timeSinceLastActivity << " seconds)" << std::endl;
                sendToClient(clientIt->first, "PING :" + _serverName);
                clientIt->second->setPingSent(true);
                clientIt->second->setPingTime(now);
            }
            // If PING was sent and no PONG received for 2 minutes, disconnect
            else if (clientIt->second->isPingSent() && (now - clientIt->second->getPingTime() > 120)) {
                std::cout << "Client " << clientIt->first << " timed out (no PONG received for " << (now - clientIt->second->getPingTime()) << " seconds)" << std::endl;
                timeoutClients.push_back(clientIt->first);
            }
        }
    }

    for (size_t i = 0; i < timeoutClients.size(); ++i) {
        close(timeoutClients[i]);
        removeClient(timeoutClients[i]);
    }
}

void Server::handleError(const std::string& error) {
    std::cerr << "Server Error: " << error << std::endl;
}

void Server::sendWelcomeMessages(int clientFd) {
    Client* client = _clients[clientFd];
    if (!client) {
        return;
    }

    std::string nick = client->getNickname();
    if (nick.empty()) {
        return;
    }

    sendToClient(clientFd, "001 " + nick + " :Welcome to the IRC Network " + nick + "!" + client->getUsername() + "@" + client->getHostname());
    sendToClient(clientFd, "002 " + nick + " :Your host is " + _serverName + ", running version " + _serverVersion);
    sendToClient(clientFd, "003 " + nick + " :This server was created " + _serverCreationDate);
    sendToClient(clientFd, "004 " + nick + " " + _serverName + " " + _serverVersion + " o o");
    sendToClient(clientFd, "005 " + nick + " CHANLIMIT=#:50 MAXCHANNELS=50 :are supported by this server");
}

std::string Server::getChannelMembers(const std::string& channel) {
    if (_channels.find(channel) == _channels.end()) {
        return "";
    }

    std::stringstream ss;
    Channel* channelPtr = _channels[channel];
    bool first = true;
    std::vector<int> members = channelPtr->getMembers();

    for (size_t i = 0; i < members.size(); ++i) {
        int memberFd = members[i];
        if (_clients.find(memberFd) == _clients.end()) {
            continue;
        }

        if (!first) {
            ss << " ";
        }
        Client* member = _clients[memberFd];
        if (channelPtr->isOperator(memberFd)) {
            ss << "@";
        }
        ss << member->getNickname();
        first = false;
    }

    return ss.str();
}

// Getters
bool Server::isRunning() const { return _running; }
std::string Server::getServerName() const { return _serverName; }
std::string Server::getServerVersion() const { return _serverVersion; }
std::string Server::getServerCreationDate() const { return _serverCreationDate; }
size_t Server::getMaxClients() const { return _maxClients; }
size_t Server::getMaxChannels() const { return _maxChannels; }
size_t Server::getClientCount() const { return _clients.size(); }
size_t Server::getChannelCount() const { return _channels.size(); }
std::time_t Server::getUptime() const { return std::time(nullptr) - _startTime; }

Client* Server::getClient(int clientFd) {
    std::map<int, Client*>::iterator it = _clients.find(clientFd);
    return it != _clients.end() ? it->second : NULL;
}

Client* Server::getClientByNickname(const std::string& nickname) {
    std::map<std::string, Client*>::iterator it = _nicknameMap.find(nickname);
    return it != _nicknameMap.end() ? it->second : NULL;
}

std::vector<Client*> Server::getClients() const {
    std::vector<Client*> result;
    std::map<int, Client*>::const_iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it) {
        result.push_back(it->second);
    }
    return result;
}

Channel* Server::getChannel(const std::string& channelName) {
    std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
    return it != _channels.end() ? it->second : NULL;
}

std::vector<Channel*> Server::getChannels() const {
    std::vector<Channel*> result;
    std::map<std::string, Channel*>::const_iterator it;
    for (it = _channels.begin(); it != _channels.end(); ++it) {
        result.push_back(it->second);
    }
    return result;
}

void Server::createChannel(const std::string& channelName, Client* creator) {
    if (_channels.find(channelName) == _channels.end()) {
        _channels[channelName] = new Channel(channelName);
        _channels[channelName]->addMember(creator->getFd());
        _channels[channelName]->addOperator(creator->getFd());
        creator->joinChannel(channelName);
    }
}

void Server::removeChannel(const std::string& channelName) {
    std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
    if (it != _channels.end()) {
        delete it->second;
        _channels.erase(it);
    }
}

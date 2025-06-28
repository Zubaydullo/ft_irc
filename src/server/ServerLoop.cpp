#include "../../include/Server.hpp"

void Server::Start() {
    if (!createSocket()) {
        std::cerr << "Failed to create server socket" << std::endl;
        return;
    }
    
    setupSignalHandlers();
    
    struct pollfd serverPoll;
    serverPoll.fd = _serverSocket;
    serverPoll.events = POLLIN;
    serverPoll.revents = 0;
    _pollfd.push_back(serverPoll);
    
    std::cout << "Server listening on port " << _port << std::endl;
    _running = true;
    
    while (_running) {
        int activity = poll(_pollfd.data(), _pollfd.size(), 1000);
        
        if (activity < 0) {
            std::cerr << "ERROR: Poll failed" << std::endl;
            break;
        }
        
        if (activity == 0) {
            continue; 
        }
        
        for (size_t i = 0; i < _pollfd.size(); ++i) {
            if (_pollfd[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                if (_pollfd[i].fd == _serverSocket) {
                    std::cerr << "ERROR: Server socket error" << std::endl;
                    _running = false;
                    break;
                } else {
                    removeClient(_pollfd[i].fd);
                    continue;
                }
            }
            
            if (_pollfd[i].revents & POLLIN) {
                if (_pollfd[i].fd == _serverSocket) {
                    acceptNewClient();
                } else {
                    handleClientData(_pollfd[i].fd);
                }
            }
            
            if (_pollfd[i].revents & POLLOUT) {
                handleClientWriteData(_pollfd[i].fd);
            }
        }
    }
    
    Stop();
}

void Server::handleClientWriteData(int clientFd) {
    if (!isValidClientFd(clientFd)) {
        return;
    }
    
    std::string& outBuffer = _Client[clientFd]->getOutBuffer();
    if (outBuffer.empty()) {
        for (size_t i = 0; i < _pollfd.size(); ++i) {
            if (_pollfd[i].fd == clientFd) {
                _pollfd[i].events = POLLIN;
                break;
            }
        }
        return;
    }
    
    ssize_t bytesSent = send(clientFd, outBuffer.c_str(), outBuffer.length(), MSG_NOSIGNAL);
    
    if (bytesSent < 0) {
        std::cerr << "Send failed for client " << clientFd << std::endl;
        removeClient(clientFd);
        return;
    }
    
    if (bytesSent == 0) {
        std::cerr << "Send returned 0 for client " << clientFd << std::endl;
        removeClient(clientFd);
        return;
    }
    
    outBuffer.erase(0, bytesSent);
    
    if (outBuffer.empty()) {
        for (size_t i = 0; i < _pollfd.size(); ++i) {
            if (_pollfd[i].fd == clientFd) {
                _pollfd[i].events = POLLIN;
                break;
            }
        }
    }
}

void Server::removeClient(int clientFd) {
    if (_Client.find(clientFd) == _Client.end()) {
        std::cout << "removeClient: Client " << clientFd << " not found" << std::endl;
        return;
    }
    
    std::cout << "Removing client " << clientFd << std::endl;
    
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); 
         it != _channels.end(); ) {
        it->second->removeMember(clientFd);
        
        if (it->second->getMemberCount() == 0) {
            std::cout << "Deleting empty channel: " << it->first << std::endl;
            delete it->second;
            std::map<std::string, Channel*>::iterator toErase = it;
            ++it;
            _channels.erase(toErase);
        } else {
            ++it;
        }
    }
    
    for (std::map<std::string, std::vector<int> >::iterator it = _inviteList.begin();
         it != _inviteList.end(); ++it) {
        std::vector<int>& invites = it->second;
        invites.erase(std::remove(invites.begin(), invites.end(), clientFd), invites.end());
    }
    
    for (std::vector<struct pollfd>::iterator it = _pollfd.begin(); 
         it != _pollfd.end(); ++it) {
        if (it->fd == clientFd) {
            std::cout << "Removing FD " << clientFd << " from poll array" << std::endl;
            _pollfd.erase(it);
            break;
        }
    }
    
    delete _Client[clientFd];
    _Client.erase(clientFd);
    close(clientFd);
    
    std::cout << "Successfully removed client " << clientFd << std::endl;
}

void Server::handleClientData(int clientFd) {
    if (!isValidClientFd(clientFd)) {
        std::cerr << "handleClientData: Invalid client FD " << clientFd << std::endl;
        return;
    }
    
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytesReceived = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesReceived < 0) {
        std::cerr << "recv() failed for client " << clientFd << std::endl;
        removeClient(clientFd);
        return;
    }
    
    if (bytesReceived == 0) {
        std::cout << "Client " << clientFd << " disconnected" << std::endl;
        removeClient(clientFd);
        return;
    }
    
    buffer[bytesReceived] = '\0';
    _Client[clientFd]->addToInBuffer(std::string(buffer));
    
    std::string& inBuffer = _Client[clientFd]->getInBuffer();
    size_t pos;
    
    while ((pos = inBuffer.find("\r\n")) != std::string::npos) {
        std::string message = inBuffer.substr(0, pos);
        inBuffer.erase(0, pos + 2);
        
        if (!message.empty()) {
            std::cout << "Received from client " << clientFd << ": " << message << std::endl;
            parseCommand(clientFd, message);
            
            if (!isValidClientFd(clientFd)) {
                return; 
            }
        }
    }
    
    while ((pos = inBuffer.find("\n")) != std::string::npos) {
        std::string message = inBuffer.substr(0, pos);
        inBuffer.erase(0, pos + 1);
        
        if (!message.empty() && message[message.length() - 1] == '\r') {
            message.erase(message.length() - 1);
        }
        
        if (!message.empty()) {
            std::cout << "Received from client " << clientFd << ": " << message << std::endl;
            parseCommand(clientFd, message);
            
            if (!isValidClientFd(clientFd)) {
                return; 
            }
        }
    }
}

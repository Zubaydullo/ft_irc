#include "../../include/Server.hpp"
#include "../../include/Client.hpp"

// Constructor
Server::Server(int port , std::string& Password):_port(port)
        ,_password(Password) ,_serverSocket(-1) , _running(false) {
    std::cout  << "Server Created with port " << _port << std::endl;
}
Server* Server::instance = NULL;
void Server::signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully..." << std::endl;
    if (instance) {
        instance->_running = false;
    }
}

void Server::setupSignalHandlers() {
    instance = this;
    signal(SIGINT, signalHandler);  
    signal(SIGTERM, signalHandler); 
    signal(SIGQUIT, signalHandler); 
    
    signal(SIGPIPE, SIG_IGN);
}
bool Server::isValidClientFd(int clientFd) {
    return _Client.find(clientFd) != _Client.end() && _Client[clientFd] != NULL;
}
bool Server::isValidChannel(const std::string& channelName) {
    return _channels.find(channelName) != _channels.end() && _channels[channelName] != NULL;
}
void Server::cleanupAllClients() {
    std::cout << "Cleaning up all clients..." << std::endl;
    for(std::map<int, Client*>::iterator it = _Client.begin(); it != _Client.end(); ++it) {
        if(it->second) {
            std::cout << "Cleaning up client " << it->first << std::endl;
            close(it->first);
            delete it->second;
            it->second = NULL;  // Prevent double deletion
        }
    }
    _Client.clear();
}

void Server::cleanupAllChannels() {
    std::cout << "Cleaning up all channels..." << std::endl;
    for(std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        if(it->second) {
            std::cout << "Cleaning up channel " << it->first << std::endl;
            delete it->second;
            it->second = NULL;  // Prevent double deletion
        }
    }
    _channels.clear();
}

// Graceful shutdown with proper cleanup
void Server::gracefulShutdown() {
    std::cout << "Performing graceful shutdown..." << std::endl;
    
    // Notify all clients about server shutdown
    for(std::map<int, Client*>::iterator it = _Client.begin(); it != _Client.end(); ++it) {
        if(it->second) {
            sendToClient(it->first, "ERROR :Server shutting down");
        }
    }
    
    cleanupAllClients();
    cleanupAllChannels();
    _inviteList.clear();
    _pollfd.clear();
    
    if(_serverSocket != -1) {
        close(_serverSocket);
        _serverSocket = -1;
    }
    
    std::cout << "Graceful shutdown completed." << std::endl;
}

// Destructor
Server::~Server(){
    std::cout << "Server destructor called..." << std::endl;
    if(_running) {
        _running = false;
    }
    gracefulShutdown();
}
// Stop the server
void Server::Stop() {
    _running = false;
    std::cout << "Server stopped" << std::endl;
}

// Get clients map
std::map<int,Client*>& Server::getClients() { 
    return _Client;
} 

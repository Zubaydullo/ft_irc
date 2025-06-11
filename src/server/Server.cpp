#include "../../include/Server.hpp"
#include "../../include/Client.hpp"

// Constructor
Server::Server(int port , std::string& Password):_port(port)
        ,_password(Password) ,_serverSocket(-1) , _running(false) {
    std::cout  << "Server Created with port " << _port << std::endl;
}

// Destructor
Server::~Server() {
    // Clean up all clients
    for(std::map<int, Client*>::iterator it = _Client.begin(); it != _Client.end(); ++it) {
        delete it->second;
    }
    _Client.clear();
    
    // Clean up all channels
    for(std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        delete it->second;
    }
    _channels.clear();
    
    // Close server socket
    if(_serverSocket != -1) {
        close(_serverSocket);
    }
    
    std::cout << "Server destroyed" << std::endl;
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
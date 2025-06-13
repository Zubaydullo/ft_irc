#include "../../include/Server.hpp"


bool Server::createSocket() {
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket < 0) {
        std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
        return false;
    }
    
    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Setsockopt SO_REUSEADDR failed: " << strerror(errno) << std::endl;
        close(_serverSocket);
        return false;
    }
    
    int flags = fcntl(_serverSocket, F_GETFL, 0);
    if (flags < 0) {
        std::cerr << "fcntl F_GETFL failed: " << strerror(errno) << std::endl;
        close(_serverSocket);
        return false;
    }
    
    if (fcntl(_serverSocket, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "fcntl F_SETFL failed: " << strerror(errno) << std::endl;
        close(_serverSocket);
        return false;
    }
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
    serverAddr.sin_port = htons(_port);
    
    if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        close(_serverSocket);
        return false;
    }
    
    if (listen(_serverSocket, SOMAXCONN) < 0) {
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        close(_serverSocket);
        return false;
    }
    
    std::cout << "Socket Created Successfully" << std::endl;
    std::cout << "Bind the socket successfully" << std::endl;
    std::cout << "Server listening on connections...." << std::endl;
    
    return true;
}
void Server::acceptNewClient() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientFd = accept(_serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
    
    if (clientFd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            std::cerr << "Warning: accept() returned EAGAIN after poll() indicated ready" << std::endl;
            return;
        }
        std::cerr << "Accept failed: " << strerror(errno) << std::endl;
        return;
    }
    
    int flags = fcntl(clientFd, F_GETFL, 0);
    if (flags < 0) {
        std::cerr << "fcntl F_GETFL failed: " << strerror(errno) << std::endl;
        close(clientFd);
        return;
    }
    
    if (fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "fcntl F_SETFL failed: " << strerror(errno) << std::endl;
        close(clientFd);
        return;
    }
    
    Client* newClient = new Client(clientFd);
    newClient->setClientIP(inet_ntoa(clientAddr.sin_addr));
    _Client[clientFd] = newClient;
    
    struct pollfd clientPoll;
    clientPoll.fd = clientFd;
    clientPoll.events = POLLIN; // Initially only listen for incoming data
    clientPoll.revents = 0;
    _pollfd.push_back(clientPoll);
    
    std::cout << "New client connected: FD " << clientFd 
              << " from " << inet_ntoa(clientAddr.sin_addr) << std::endl;
}

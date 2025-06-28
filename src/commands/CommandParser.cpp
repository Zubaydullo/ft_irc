#include "../../include/Server.hpp"

void Server::parseCommand(int clientFd, const std::string& message){
    if(!isValidClientFd(clientFd)) {
        std::cerr << "parseCommand: Invalid client FD " << clientFd << std::endl;
        return;
    }
    
    if(message.empty()) {
        return;  
    }
    
    if(message.length() > 512) {  
        std::cerr << "parseCommand: Message too long from client " << clientFd << std::endl;
        sendToClient(clientFd, "ERROR :Message too long");
        return;
    }
    
    std::cout << "Parse Command : " << message << std::endl;
  
    std::istringstream iss(message);
    std::string command;
    
    if(!(iss >> command)) {
        return;
    }
    
    if(command.length() > 32) {  
        sendToClient(clientFd, "421 " + _Client[clientFd]->getNickname() + " " + command + " :Unknown command");
        return;
    }
    
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);
    
    if(command == "PASS"){
       handelPass(clientFd,iss);
    }else if(command == "NICK"){
       handelNick(clientFd,iss);
    }else if(command == "USER"){
        handelUser(clientFd,iss);
    }else if(command == "JOIN"){
        handleJoin(clientFd,iss);
    }else if(command == "PART"){
        handlePart(clientFd,iss);
    }else if(command == "PRIVMSG") {
        handlePrivmsg(clientFd, iss);
    }else if(command == "MODE"){
         handleMode(clientFd,iss);
    }else if(command == "KICK"){
         handleKick(clientFd,iss);
    }else if(command == "TOPIC"){
        handleTopic(clientFd,iss);
    }else if(command == "NAMES"){
        handleNames(clientFd,iss);
    }else if(command == "INVITE"){
        handleInvite(clientFd,iss);
    } else if (command == "PING") {
        std::string token;
        iss >> token;
        sendToClient(clientFd, "PONG :" + token);
    } else if (command == "QUIT") {
        removeClient(clientFd);
        return;
    } else if (command == "CAP") {
        std::string subcommand;
        iss >> subcommand;
        if (subcommand == "LS") {
            sendToClient(clientFd, ":localhost CAP * LS :");
        } else if (subcommand == "END") {
        }
    } else if (command.find("DCC") != std::string::npos) {
        handleDCC(clientFd, iss);
    } else {
        if(_Client.find(clientFd) != _Client.end() && _Client[clientFd]) {
            sendToClient(clientFd, "421 " + _Client[clientFd]->getNickname() + " " + command + " :Unknown command");
        }
        std::cout << "Unknown command: " << command << std::endl;
    }

    if(!isValidClientFd(clientFd)) {
        return;  
    }

}

int Server::findClientByNick(const std::string& nickname) {
    for(std::map<int, Client*>::iterator it = _Client.begin(); it != _Client.end(); ++it) {
        if(it->second->getNickname() == nickname) {
            return it->first;
        }
    }
    return -1;
}

void Server::sendToClientRaw(int clientFd, const std::string& rawMessage) {
    if (!isValidClientFd(clientFd)) {
        std::cerr << "Warning: Attempted to send to invalid client " << clientFd << std::endl;
        return;
    }
    
    // Raw message already formatted - just add final \r\n
    std::string fullMessage = rawMessage + "\r\n";
    
    // Try to send immediately
    ssize_t bytesSent = send(clientFd, fullMessage.c_str(), fullMessage.length(), MSG_NOSIGNAL | MSG_DONTWAIT);
    
    if (bytesSent < 0) {
        // Would block or error - add to buffer and enable POLLOUT
        _Client[clientFd]->addToOutBuffer(fullMessage);
        enablePollOut(clientFd);
        std::cout << "Buffered raw message for client " << clientFd << std::endl;
        return;
    }
    
    if (bytesSent == 0) {
        std::cerr << "Send returned 0 for client " << clientFd << std::endl;
        removeClient(clientFd);
        return;
    }
    
    if ((size_t)bytesSent < fullMessage.length()) {
        std::string remaining = fullMessage.substr(bytesSent);
        _Client[clientFd]->addToOutBuffer(remaining);
        enablePollOut(clientFd);
        std::cout << "Partial send to client " << clientFd << ", buffered remaining " << remaining.length() << " bytes" << std::endl;
    }
    
    std::cout << "Sent raw message to Client " << clientFd << std::endl;
}

void Server::sendToClient(int clientFd, const std::string& message) {
    if (!isValidClientFd(clientFd)) {
        std::cerr << "Warning: Attempted to send to invalid client " << clientFd << std::endl;
        return;
    }
    
    std::string fullMessage = message + "\r\n";
    
    ssize_t bytesSent = send(clientFd, fullMessage.c_str(), fullMessage.length(), MSG_NOSIGNAL | MSG_DONTWAIT);
    
    if (bytesSent < 0) {
        _Client[clientFd]->addToOutBuffer(fullMessage);
        enablePollOut(clientFd);
        std::cout << "Buffered message for client " << clientFd << ": " << message << std::endl;
        return;
    }
    
    if (bytesSent == 0) {
        std::cerr << "Send returned 0 for client " << clientFd << std::endl;
        removeClient(clientFd);
        return;
    }
    
    if ((size_t)bytesSent < fullMessage.length()) {
        std::string remaining = fullMessage.substr(bytesSent);
        _Client[clientFd]->addToOutBuffer(remaining);
        enablePollOut(clientFd);
        std::cout << "Partial send to client " << clientFd << ", buffered remaining " << remaining.length() << " bytes" << std::endl;
    }
    
    std::cout << "Sent to Client " << clientFd << ": " << message << std::endl;
}

void Server::enablePollOut(int clientFd) {
    for (size_t i = 0; i < _pollfd.size(); ++i) {
        if (_pollfd[i].fd == clientFd) {
            _pollfd[i].events |= POLLOUT; 
            break;
        }
    }
}

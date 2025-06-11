#include "../../include/Server.hpp"

void Server::parseCommand(int clientFd, const std::string& message){
    std::cout << "Parse Command : " << message << std::endl;
  
    std::istringstream iss(message);
    std::string command;
    iss >> command;
    
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
        // Handle capability negotiation - common in modern clients
        std::string subcommand;
        iss >> subcommand;
        if (subcommand == "LS") {
            sendToClient(clientFd, ":localhost CAP * LS :");
        } else if (subcommand == "END") {
            // Client finished capability negotiation
        }
    } else if (command.find("DCC") != std::string::npos) {
        handleDCC(clientFd, iss);
    } else {
        sendToClient(clientFd, "421 " + _Client[clientFd]->getNickname() + " " + command + " :Unknown command");
        std::cout << "Unknown command: " << command << std::endl;
    }

    // Check if client should be registered after processing commands
    if(_Client[clientFd]->isAuthenticated() && 
       !_Client[clientFd]->getNickname().empty() && 
       !_Client[clientFd]->getUsername().empty() && 
       !_Client[clientFd]->isRegistered()) {
        
        _Client[clientFd]->setRegistered(true);
        std::string nick = _Client[clientFd]->getNickname();
        sendToClient(clientFd, ":localhost 001 " + nick + " :Welcome to the IRC Server!");
        sendToClient(clientFd, ":localhost 002 " + nick + " :Your host is localhost");
        sendToClient(clientFd, ":localhost 003 " + nick + " :Server created recently");
        sendToClient(clientFd, ":localhost 004 " + nick + " localhost v1.0 o o");
        std::cout << "Client " << clientFd << " (" << nick << ") is now fully registered" << std::endl;
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

void Server::sendToClient(int clientFd, const std::string& message){
    if(!isValidClientFd(clientFd)) {
        std::cerr << "Warning: Attempted to send to invalid client " << clientFd << std::endl;
        return;
    }
    
    std::string msg = message + "\r\n";
    ssize_t sent = send(clientFd, msg.c_str(), msg.length(), 0);
    if(sent < 0) {
        std::cerr << "Error sending to client " << clientFd << std::endl;
        removeClient(clientFd);   
        return;
    }
    std::cout << "Sent to Client " << clientFd << ": " << message << std::endl; 
}

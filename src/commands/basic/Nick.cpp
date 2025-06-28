#include "../../../include/Server.hpp"

void Server::handelNick(int clientFd, std::istringstream& iss) { 
    std::string nickName;
    iss >> nickName;
    
    if (nickName.empty()) {
        std::string currentNick = _Client[clientFd]->getNickname();
        if (currentNick.empty()) currentNick = "*";
        sendToClient(clientFd, "431 " + currentNick + " :No nickname given");
        return;
    }
    
    std::cout << "Client :" << clientFd << " wants nickname: " << nickName << std::endl;
    
    for (std::map<int, Client*>::iterator it = _Client.begin(); it != _Client.end(); ++it) {
        if (it->second->getNickname() == nickName && it->first != clientFd) {
            std::string currentNick = _Client[clientFd]->getNickname();
            if (currentNick.empty()) currentNick = "*";
            sendToClient(clientFd, "433 " + currentNick + " " + nickName + " :Nickname is already in use");
            return;
        }
    }
    
    _Client[clientFd]->setNickname(nickName);
    std::cout << "Client " << clientFd << " nickname set to: " << nickName << std::endl;
    
    if (_Client[clientFd]->isAuthenticated() && 
        !_Client[clientFd]->getNickname().empty() && 
        !_Client[clientFd]->getUsername().empty() && 
        !_Client[clientFd]->isRegistered()) {
        
        _Client[clientFd]->setRegistered(true);
        
        std::string nick = _Client[clientFd]->getNickname();
        std::cout << "ClientFd " << clientFd << " (" << nick << ") is now fully registered" << std::endl;
        
        std::string welcomeBlock = 
            ":localhost 001 " + nick + " :Welcome to the IRC Server, " + nick + "!\r\n"
            ":localhost 002 " + nick + " :Your host is localhost, running version 1.0\r\n"
            ":localhost 003 " + nick + " :This server was created recently\r\n"
            ":localhost 004 " + nick + " localhost 1.0 o o";
        
        sendToClientRaw(clientFd, welcomeBlock);
        
        std::cout << "Welcome messages buffered for " << nick << std::endl;
    }
}

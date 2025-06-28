#include "../../../include/Server.hpp"

void Server::handelUser(int clientFd, std::istringstream& iss) {
    std::string username, hostname, servername, realname;
    iss >> username >> hostname >> servername;
    std::getline(iss, realname);
    
    if (!realname.empty() && realname[0] == ' ') realname = realname.substr(1);
    if (!realname.empty() && realname[0] == ':') realname = realname.substr(1);

    if (username.empty() || hostname.empty() || servername.empty() || realname.empty()) {
        sendToClient(clientFd, "461 " + _Client[clientFd]->getNickname() + " USER :Not enough parameters");
        return;
    }
    
    for (std::map<int, Client*>::iterator it = _Client.begin(); it != _Client.end(); ++it) {
        if (it->second->getUsername() == username && it->first != clientFd) {
            sendToClient(clientFd, "433 " + _Client[clientFd]->getNickname() + " " + username + " :Username is already taken");
            return;
        }
    }
    
    std::cout << "ClientFd " << clientFd << " user info: " << username << std::endl;

    _Client[clientFd]->setUsername(username);
    _Client[clientFd]->setRealname(realname);
  
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

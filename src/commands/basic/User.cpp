#include "../../../include/Server.hpp"

void Server::handelUser(int clinetFd , std::istringstream& iss){
     
     std::string username, hostname , servername, realname;
     iss  >> username >> hostname >> servername;
     std::getline(iss, realname);

    if(username.empty() || hostname.empty() || servername.empty() || realname.empty()){
        sendToClient(clinetFd , "461 " + _Client[clinetFd]->getNickname() + " :Not enough parameters");
        return;
    }
    // check if the username is already taken
    for(std::map<int, Client*>::iterator it = _Client.begin() ; it != _Client.end() ; ++it){
        if(it->second->getUsername() == username){
            sendToClient(clinetFd , "433 " + _Client[clinetFd]->getUsername() + " :Username is already taken");
            return;
        }
    }
    std::cout << "ClinedFd" << clinetFd << " user info : " << username << std::endl;

    _Client[clinetFd]->setUsername(username);
    _Client[clinetFd]->setRealname(realname);
  
   if(_Client[clinetFd]->isAuthenticated() && !_Client[clinetFd]->getNickname().empty()){
    _Client[clinetFd]->setRegistered(true);
    std::cout << "ClinetFd " << clinetFd << " is now fully registered "  << std::endl;
    
    std::string nick = _Client[clinetFd]->getNickname();
    sendToClient(clinetFd, ":localhost 001 " + nick + " :Welcome to the IRC Server!");    
    sendToClient(clinetFd, ":localhost 002 " + nick + " :Your host is localhost");    
    sendToClient(clinetFd, ":localhost 003 " + nick + " :Server created recently");    
    sendToClient(clinetFd, ":localhost 004 " + nick + " localhost v1.0 o o");
   }
}

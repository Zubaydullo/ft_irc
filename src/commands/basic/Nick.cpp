#include "../../../include/Server.hpp"

void Server::handelNick(int clientFd, std::istringstream& iss)
{ 
    std::string nickName;
    iss >> nickName;
    if(nickName.empty()){
        sendToClient(clientFd , "431 " + _Client[clientFd]->getNickname() + " :No nickname given");
        return;
    }
    std::cout << "Clinet  :"  << clientFd << " Wants  nickname:" << nickName << std::endl;
    for(std::map<int, Client*>::iterator it = _Client.begin() ; it != _Client.end() ; ++it){
        if(it->second->getNickname() == nickName){
            sendToClient(clientFd , "433 " + _Client[clientFd]->getNickname() + " :Nickname is already taken");
            return;
        }
    }
    _Client[clientFd]->setNickname(nickName);     
}

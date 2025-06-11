#include "../../../include/Server.hpp"

void Server::handleNames(int clientFd , std::istringstream& iss){
    std::string channelName;
    iss >> channelName;
    if(_channels.find(channelName) == _channels.end()){
        sendToClient(clientFd , "403 " + _Client[clientFd]->getNickname() + " " + channelName + " :No such channel");
        return;
    }
    
    std::vector<int> members = _channels[channelName]->getMembers();
    std::string namesList = "";
    
    for(std::vector<int>::iterator it = members.begin() ; it != members.end() ; ++it){
        if(!namesList.empty()) {
            namesList += " ";
        }
        // Add @ prefix for operators
        if(_channels[channelName]->isOperator(*it)) {
            namesList += "@";
        }
        namesList += _Client[*it]->getNickname();
    }
    
    sendToClient(clientFd, "353 " + _Client[clientFd]->getNickname() + " = " + channelName + " :" + namesList);
    sendToClient(clientFd, "366 " + _Client[clientFd]->getNickname() + " " + channelName + " :End of /NAMES list");
}

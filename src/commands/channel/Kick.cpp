#include "../../../include/Server.hpp"

void Server::handleKick(int clientFd , std::istringstream& iss){ 
    std::string channelName, targetNick, reason;
    iss >> channelName >> targetNick;
    std::getline(iss, reason);

    if (!reason.empty() && reason[0] == ' ') reason = reason.substr(1);
    if (!reason.empty() && reason[0] == ':') reason = reason.substr(1);
    if (reason.empty()) reason = "No reason given";

    if(_channels.find(channelName) == _channels.end()){
         sendToClient(clientFd , "403 " + _Client[clientFd]->getNickname()  + 
                 channelName + ":No such channel ");
        return;
    }
    if(!_channels[channelName]->isMember(clientFd))
        {
             sendToClient(clientFd,"442 " + _Client[clientFd]->getNickname() + " " + channelName + "You're not that  channel " );
            return;
        }
     if(!_channels[channelName]->isOperator(clientFd)){ 
           sendToClient(clientFd, "482 " + _Client[clientFd]->getNickname() + channelName + "You are not the channel operator");
        return;
     } 
     int targetFd = -1;
    for (std::map<int, Client*>::iterator it = _Client.begin(); it != _Client.end(); ++it) {
        if (it->second->getNickname() == targetNick) {
            targetFd = it->first;
            break;
        }
    }
    if(targetFd == -1 ){ 
        sendToClient(clientFd , "441 " + _Client[clientFd]->getNickname() 
                + " " + targetNick + " " + channelName + "The aren't on the that channel" );
    }
    std::string kickerNick = _Client[clientFd]->getNickname(); 
    std::vector<int> members = _channels[channelName]->getMembers();
    for(std::vector<int>::iterator it = members.begin() ; it != members.end() ; ++it){
         sendToClient(*it, ":" + kickerNick + " KICK " + channelName + " " + targetNick + " :" + reason);
    }
    _channels[channelName]->removeMember(targetFd);
    std::cout <<kickerNick  <<   " kicked" << targetNick << "from " << channelName  << " ( " << reason << ")" <<  std::endl;
}

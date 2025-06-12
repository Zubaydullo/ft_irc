#include "../../../include/Server.hpp"

void Server::handleInvite(int clientFd , std::istringstream& iss){
     std::string targetNick , channelName;
     iss >> targetNick >> channelName;
     
     if(targetNick.empty() || channelName.empty()){ 
               sendToClient(clientFd, "461 " + _Client[clientFd]->getNickname() + " INVITE :Not enough parameters");       
         return;
     }
     if(_channels.find(channelName) == _channels.end()){
             sendToClient(clientFd, "403 " + _Client[clientFd]->getNickname() + " " + channelName + " :No such channel");
         return ;
     }
     if(!_channels[channelName]->isMember(clientFd)){
            sendToClient(clientFd, "442 " + _Client[clientFd]->getNickname() + " " + channelName + " :You're not on that channel"); 
         return;
     }
     if(_channels[channelName]->isInviteOnly() && !_channels[channelName]->isOperator(clientFd)){
              sendToClient(clientFd, "482 " + _Client[clientFd]->getNickname() + " " + channelName + " :You're not channel operator");
         return;
     }
    int targetFd = findClientByNick(targetNick);
    if(targetFd == -1){ 
         sendToClient(clientFd, "401 " + _Client[clientFd]->getNickname() + " " + targetNick + " :No such nick");
        return;
    }
    if(_channels[channelName]->isMember(targetFd)){
          sendToClient(clientFd, "443 " + _Client[clientFd]->getNickname() + " " + targetNick + " " + channelName + " :is already on channel");
        return;
    }
    _inviteList[channelName].push_back(targetFd);

    std::string inviterNick = _Client[clientFd]->getNickname();
    sendToClient(targetFd, ":" + inviterNick + " INVITE " + targetNick + " " + channelName);
    sendToClient(clientFd, "341 " + inviterNick + " " + targetNick + " " + channelName);
    std::cout << inviterNick << " invited " << targetNick << " to " + channelName << std::endl;
}

#include "../../../include/Server.hpp"

void Server::handleJoin(int clientFd , std::istringstream& iss){
      
    std::string channelName,password;
    iss >> channelName >> password;
    
    if (channelName.empty() || channelName[0] != '#') {
        sendToClient(clientFd, "403 " + _Client[clientFd]->getNickname() + " " + channelName + " :Invalid channel name");
        return;
    }
    if (channelName.length() > 50) {
        sendToClient(clientFd, "403 " + _Client[clientFd]->getNickname() + " " + channelName + " :Channel name too long");
        return;
    }
    if (channelName.find(' ') != std::string::npos) {
        sendToClient(clientFd, "403 " + _Client[clientFd]->getNickname() + " " + channelName + " :Channel name cannot contain spaces");
        return;
    }
    if (channelName.length() < 2 || channelName.find_first_not_of("#") == std::string::npos) {
        sendToClient(clientFd, "403 " + _Client[clientFd]->getNickname() + " " + channelName + " :Invalid channel name");
        return;
    }
    

    if(!_Client[clientFd]->isRegistered()){
         
        sendToClient(clientFd , "451 :You have not registered");
        return;
    }

        std::cout << "Client  " << clientFd << " Wants to jion channel: " << channelName << std::endl; 
    
       if(!isValidChannel(channelName)){ // if both equal to end channel don't exisiting 
        if (channelName=="#bot" && _Client[clientFd]->getNickname()!= "ircbot")
        {
            sendToClient(clientFd, "403 " + _Client[clientFd]->getNickname() + " " + channelName + " :This channel is reserved for the bot");
            return;
        }                               
        
         _channels[channelName] = new Channel(channelName);
         std::cout << "Created new channel : " << channelName << std::endl;
    }
        Channel * channel= _channels[channelName];

    if(channel->isMember(clientFd)){
          sendToClient(clientFd, "443 " + _Client[clientFd]->getNickname() + " " + channelName + " :is already on channel");
        return;
    }
    if(channel->isInviteOnly()){
        bool isInvited = false;
        std::vector<int>& invites = _inviteList[channelName];
        for(std::vector<int>::iterator it = invites.begin() ; it != invites.end(); ++it){
            if(*it == clientFd){ 
                isInvited = true;
                invites.erase(it);
                break;
            }
        }
        if(!isInvited){
         sendToClient(clientFd, "473 " + _Client[clientFd]->getNickname() + " " + channelName + " :Cannot join channel (+i)");
        return;    
        }
    }
    if(!channel->getPassword().empty() && password != channel->getPassword()){
            sendToClient(clientFd, "475 " + _Client[clientFd]->getNickname() + " " + channelName + " :Cannot join channel (+k)");
        return; 
    }
    if(channel->getUserLimit() > 0 && channel->getMemberCount() >= channel->getUserLimit()){
         sendToClient(clientFd, "471 " + _Client[clientFd]->getNickname() + " " + channelName + " :Cannot join channel (+l)");
        return;
    }

        // after passs all the checkes
    
      bool isFirstMember = (_channels[channelName]->getMemberCount() == 0);
    _channels[channelName]->addMember(clientFd);
     if(isFirstMember){
         _channels[channelName]->addOperator(clientFd);
          std::cout << "Client " << clientFd << " is now operator of " << channelName << std::endl;
    }
    std::string nick = _Client[clientFd]->getNickname();
    std::vector<int> members = _channels[channelName]->getMembers();
    for(std::vector<int>::iterator it = members.begin() ; it != members.end() ; ++it){

        if(*it != clientFd) sendToClient(*it , ":" + nick +  " JOIN " + channelName);
         
    }
    sendToClient(clientFd , ":" + nick + " JOIN " + channelName);
    
    // now we  need to  send the info to the channel 
    sendToClient(clientFd , "353 " + nick + " = " + channelName + " : " + nick);
    sendToClient(clientFd , "366 " + nick + " " + channelName + " :End of /NAMES  list");
}

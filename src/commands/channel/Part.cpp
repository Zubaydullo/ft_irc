#include "../../../include/Server.hpp"

void Server::handlePart(int clientFd, std::istringstream& iss){

    std::string channelName;
    iss >> channelName;

    if(_channels.find(channelName) != _channels.end()){ 
     
        std::string nick = _Client[clientFd]->getNickname();
        
        std::vector<int> members = _channels[channelName]->getMembers();
        for(std::vector<int>::iterator  it = members.begin() ; it != members.end(); ++ it){ 
            sendToClient(*it , " : " + nick + " PART " + channelName);
            
        }
        _channels[channelName]->removeMember(clientFd);
        std::cout << "Client " << clientFd << " left channel: " << channelName << std::endl;

        if(_channels[channelName]->getMemberCount() == 0){
             delete _channels[channelName];
             _channels.erase(channelName);
             std::cout << "Deleted empty channel:  " << channelName << std::endl;
        }
    }
}

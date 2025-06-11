#include "../../../include/Server.hpp"
void Server::handlePrivmsg(int clientFd , std::istringstream& iss) {
    std::string target , message;
    iss >> target;
        
    std::getline(iss, message);
    if(!message.empty() && message[0] == ' ') message = message.substr(1);
    if(!message.empty() && message[0] == ':')  message = message.substr(1);
    
    if (message.find("DCC SEND") != std::string::npos || message.find("\001DCC SEND") != std::string::npos) {
        std::istringstream dccStream(target + " " + message);
        handleDCC(clientFd, dccStream);
        return;
    }

    std::string senderNick = _Client[clientFd]->getNickname();
    std::cout << senderNick << " sends to " << target << " : " << message << std::endl;
    
    if(target[0] == '#'){
        if(_channels.find(target) == _channels.end()){
            sendToClient(clientFd, "403 " + senderNick + " " + target + " :No such channel");
            return;
        }
        
        if(!_channels[target]->isMember(clientFd)){
            sendToClient(clientFd, "404 " + senderNick + " " + target + " :Cannot send to channel");
            return;
        }   
        
        std::vector<int> members = _channels[target]->getMembers();
        for(std::vector<int>::iterator it = members.begin(); it != members.end(); ++it){
            int membersFd = *it;
            if(membersFd != clientFd){  
                sendToClient(membersFd, ":" + senderNick + " PRIVMSG " + target + " :" + message);
            }
        }
    }else{
        for(std::map<int, Client*>::iterator it = _Client.begin(); it != _Client.end(); ++it){
            if(it->second->getNickname() == target){
                sendToClient(it->first, ":" + senderNick + " PRIVMSG " + target + " :" + message);
                std::cout << "Private message sent from " << senderNick << " to " << target << std::endl;
                return;
            }
        }
        sendToClient(clientFd, "401 " + senderNick + " " + target + " :No such nick/channel");
    }
}

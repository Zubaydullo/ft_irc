#include "../../../include/Server.hpp"

void Server::handleTopic(int clientFd , std::istringstream& iss){
   std::string channelName, topic;
    iss >> channelName;
    std::getline(iss,topic);
    
    std::cout << "DEBUG: The value of topic is " << topic << std::endl; 
    if (!topic.empty() && topic[0] == ' ') topic = topic.substr(1);
    if (!topic.empty() && topic[0] == ':') topic = topic.substr(1);
      
    std::cout << "DEBUG: The value of topic is after feltring : " << topic << std::endl; 
    
    if (_channels.find(channelName) == _channels.end()) {
        sendToClient(clientFd, "403 " + _Client[clientFd]->getNickname() + " " + channelName + " :No such channel");
        return;
    }
    
    if (!_channels[channelName]->isMember(clientFd)) {
        sendToClient(clientFd, "442 " + _Client[clientFd]->getNickname() + " " + channelName + " :You're not on that channel");
        return;
    }
    std::string nick = _Client[clientFd]->getNickname();
    std::cout << "DEBUG : This the length of the topic :  " << topic.length() << std::endl;
    std::cout << "DEBUD : this t the empby value " << (topic.empty() ? true : false) << std::endl;
    if(topic.length() <= 1){
        std::string currentTopic = _channels[channelName]->getTopic();
        if (currentTopic.empty()) {
            sendToClient(clientFd, "331 " + nick + " " + channelName + " :No topic is set");
        } else {
            sendToClient(clientFd, "332 " + nick + " " + channelName + " :" + currentTopic);
        }
    }else {
        if(_channels[channelName]->isTopicRestricted() && !_channels[channelName]->isOperator(clientFd)){
               sendToClient(clientFd, "482 " + nick + " " + channelName + " :You're not channel operator");
            return;
        } 
        std::cout << "enter here to check  after change the topic " << std::endl;
        
      _channels[channelName]->setTopic(topic);
      
      std::vector<int> members = _channels[channelName]->getMembers();
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " TOPIC " + channelName + " :" + topic);
        }
        
        std::cout << nick << " changed topic of " << channelName << " to: " << topic << std::endl;
    
    }
}

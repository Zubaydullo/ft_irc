#include "../../include/Channel.hpp"




Channel::Channel(const std::string& name) : _name(name){
        
    _topic = "";
    _password = "";
    _inviteOnly   = false;
    _topicRestricted = true;
    _userLimit = 0;
}


std::string Channel::getName() const {
   
    return _name;
}

std::string Channel::getTopic()const { 
    
    return _topic;
}


std::vector<int>  Channel::getMembers() const{
    return _members;
}

int  Channel::getMemberCount()const {
     return _members.size();
}

void  Channel::setTopic(const std::string& topic){
     _topic = topic;
}

void Channel::setPassword(const std::string& password){
     _password = password;
}

void  Channel::addMember(int ClientFd){
     if(!isMember(ClientFd)){
         _members.push_back(ClientFd); 
     }
}

void Channel::removeMember(int ClientFd){
     _members.erase(std::remove(_members.begin() , _members.end(), ClientFd) ,_members.end());
    _operators.erase(std::remove(_operators.begin() , _operators.end() , ClientFd ) , _operators.end());
    
}


bool Channel::isMember(int clientFd) const{
    
        return std::find(_members.begin(), _members.end() , clientFd) != _members.end();
        
}

bool Channel::isOperator(int clientFd)const {
      return std::find(_operators.begin() , _operators.end() , clientFd) != _operators.end();
}
Channel::~Channel()
{
    _members.clear();
    _operators.clear();
}

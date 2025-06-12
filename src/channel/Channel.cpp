#include "../../include/Channel.hpp"




Channel::Channel(const std::string& name) : _name(name){
        
     std::cout << "Channel " << _name << " created" << std::endl;
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
bool Channel::isInviteOnly()const { 
    return _inviteOnly;
}

bool Channel::isTopicRestricted()const {
     return _topicRestricted;
}

int Channel::getUserLimit()const {
     return _userLimit;
}

std::string Channel::getPassword()const{ 
    return _password;
}

void  Channel::setTopic(const std::string& topic){
     _topic = topic;
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


// imeplement the operators

bool Channel::isOperator(int clientFd) const
{
    for (std::vector<int>::const_iterator it = _operators.begin(); it != _operators.end(); ++it) {
        if (*it == clientFd) {
            return true;
        }
    }
    return false;
}

void Channel::setTopicRestricted(bool mode){ 
       _topicRestricted = mode;

}

void Channel::setInviteOnly(bool mode){ 
    _inviteOnly = mode;
}

void  Channel::setUserLimit(int userLimit){
    _userLimit = userLimit; 
}

void Channel::addOperator(int clientFd)
{
    if (!isOperator(clientFd)) {
        _operators.push_back(clientFd);
    }
}
void Channel::setPassword(const std::string& password){
     _password = password;
}


void Channel::removeOperator(int clientFd)
{
    for (std::vector<int>::iterator it = _operators.begin(); it != _operators.end(); ++it) {
        if (*it == clientFd) {
            _operators.erase(it);
            break;
        }
    }
}


Channel::~Channel()
{
    _members.clear();
    _operators.clear();
}

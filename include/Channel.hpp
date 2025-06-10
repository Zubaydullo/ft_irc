#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>


class Channel {
    
     private:
        std::string _name;
        std::string _topic;
        std::vector <int> _members;  //  members
        std::vector <int> _operators; //  root
        std::string _password;
        bool _inviteOnly;
        bool  _topicRestricted;
        int _userLimit;
     public:
        Channel(const std::string& name);
        ~Channel();
        std::string   getName() const;
        std::string   getTopic() const;

      void addMember(int clientFd);
      void removeMember(int clientFd);
      bool isMember(int clientFd) const;
      bool isOperator(int clientFd) const;
      bool isInviteOnly() const;
      bool isTopicRestricted() const;
      std::string  getPassword()const;
      int getUserLimit() const;

      void addOperator(int clientFd);
      void removeOperator(int clientFd);
    std::vector<int> getMembers() const ;
    int getMemberCount() const ;
     
    void setTopicRestricted(bool mode);
    void setInviteOnly(bool mode);
    void setUserLimit(int userLimit);
    void setTopic(const std::string& topic);
    void setPassword(const std::string& password);
};


#endif

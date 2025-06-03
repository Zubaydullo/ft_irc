#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <map>
#include <vector>
#include <string>



class Channel {
    
     private:
        std::string _name;
        std::string _topic;
        std::vector <int> _members;
        std::vector <int> _operators;
        std::string _password;
        bool _inviteOnly;
        bool  _topicRestricted;
        bool _userLimit;
     public:
        Channel(const std::string& name);
        ~Channel();
        std::string   getName() const;
        std::string   getTopic() const;

      void addMember(int clientFd);
      void removeMember(int clientFd);
      bool isMember(int clientFd) const;
      bool isOperator(int clientFd) const;
    std::vector<int> getMembers() const ;
    int getMemberCount() const ;

    void setTopic(const std::string& topic);
    void setPassword(const std::string& password);
};


#endif

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include <set>
#include <map>

class Channel {
private:
    std::string _name;
    std::string _topic;
    std::string _mode;
    std::vector<int> _members;
    std::set<int> _operators;
    std::set<int> _banned;
    std::set<int> _invited;
    bool _topicProtected;
    bool _inviteOnly;
    int _userLimit;
    std::string _key;

public:
    Channel(const std::string& name);
    ~Channel();

    // Channel information
    std::string getName() const;
    std::string getTopic() const;
    std::string getMode() const;
    int getMemberCount() const;
    bool isTopicProtected() const;
    bool isInviteOnly() const;
    int getUserLimit() const;
    std::string getKey() const;

    // Member management
    void addMember(int clientFd);
    void removeMember(int clientFd);
    bool hasMember(int clientFd) const;
    std::vector<int> getMembers() const;
    bool isOperator(int clientFd) const;
    void addOperator(int clientFd);
    void removeOperator(int clientFd);
    bool isBanned(int clientFd) const;
    void ban(int clientFd);
    void unban(int clientFd);
    bool isInvited(int clientFd) const;
    void invite(int clientFd);
    void uninvite(int clientFd);

    // Channel settings
    void setTopic(const std::string& topic);
    void setMode(const std::string& mode);
    void setTopicProtected(bool isProtected);
    void setInviteOnly(bool inviteOnly);
    void setUserLimit(int limit);
    void setKey(const std::string& key);
};

#endif

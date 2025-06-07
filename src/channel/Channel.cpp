#include "../../include/Channel.hpp"

Channel::Channel(const std::string& name)
    : _name(name), _topic(""), _mode(""), _topicProtected(false),
      _inviteOnly(false), _userLimit(0) {
}

Channel::~Channel() {
}

// Channel information
std::string Channel::getName() const {
    return _name;
}

std::string Channel::getTopic() const {
    return _topic;
}

std::string Channel::getMode() const {
    return _mode;
}

int Channel::getMemberCount() const {
    return _members.size();
}

bool Channel::isTopicProtected() const {
    return _topicProtected;
}

bool Channel::isInviteOnly() const {
    return _inviteOnly;
}

int Channel::getUserLimit() const {
    return _userLimit;
}

std::string Channel::getKey() const {
    return _key;
}

// Member management
void Channel::addMember(int clientFd) {
    if (!hasMember(clientFd)) {
        _members.push_back(clientFd);
    }
}

void Channel::removeMember(int clientFd) {
    _members.erase(
        std::remove(_members.begin(), _members.end(), clientFd),
        _members.end()
    );
    _operators.erase(clientFd);
}

bool Channel::hasMember(int clientFd) const {
    return std::find(_members.begin(), _members.end(), clientFd) != _members.end();
}

std::vector<int> Channel::getMembers() const {
    return _members;
}

bool Channel::isOperator(int clientFd) const {
    return _operators.find(clientFd) != _operators.end();
}

void Channel::addOperator(int clientFd) {
    if (hasMember(clientFd)) {
        _operators.insert(clientFd);
    }
}

void Channel::removeOperator(int clientFd) {
    _operators.erase(clientFd);
}

bool Channel::isBanned(int clientFd) const {
    return _banned.find(clientFd) != _banned.end();
}

void Channel::ban(int clientFd) {
    _banned.insert(clientFd);
    if (hasMember(clientFd)) {
        removeMember(clientFd);
    }
}

void Channel::unban(int clientFd) {
    _banned.erase(clientFd);
}

bool Channel::isInvited(int clientFd) const {
    return _invited.find(clientFd) != _invited.end();
}

void Channel::invite(int clientFd) {
    _invited.insert(clientFd);
}

void Channel::uninvite(int clientFd) {
    _invited.erase(clientFd);
}

// Channel settings
void Channel::setTopic(const std::string& topic) {
    _topic = topic;
}

void Channel::setMode(const std::string& mode) {
    _mode = mode;
    
    // Parse mode string and update channel settings
    for (size_t i = 0; i < mode.length(); i++) {
        if (mode[i] == '+') {
            i++;
            while (i < mode.length() && mode[i] != '+' && mode[i] != '-') {
                switch (mode[i]) {
                    case 't':
                        _topicProtected = true;
                        break;
                    case 'i':
                        _inviteOnly = true;
                        break;
                    case 'k':
                        if (i + 1 < mode.length()) {
                            _key = mode.substr(i + 1);
                            i = mode.length(); // Skip the rest
                        }
                        break;
                    case 'l':
                        if (i + 1 < mode.length()) {
                            _userLimit = std::stoi(mode.substr(i + 1));
                            i = mode.length(); // Skip the rest
                        }
                        break;
                }
                i++;
            }
            i--; // Adjust for the loop increment
        } else if (mode[i] == '-') {
            i++;
            while (i < mode.length() && mode[i] != '+' && mode[i] != '-') {
                switch (mode[i]) {
                    case 't':
                        _topicProtected = false;
                        break;
                    case 'i':
                        _inviteOnly = false;
                        break;
                    case 'k':
                        _key = "";
                        break;
                    case 'l':
                        _userLimit = 0;
                        break;
                }
                i++;
            }
            i--; // Adjust for the loop increment
        }
    }
}

void Channel::setTopicProtected(bool isProtected) {
    _topicProtected = isProtected;
}

void Channel::setInviteOnly(bool inviteOnly) {
    _inviteOnly = inviteOnly;
}

void Channel::setUserLimit(int limit) {
    _userLimit = limit;
}

void Channel::setKey(const std::string& key) {
    _key = key;
}

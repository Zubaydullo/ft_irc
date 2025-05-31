#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client
{
private:
    int _fd;
    std::string _nickname;
    std::string _username;
    bool _authenticated;

public:
    Client(int fd);
    ~Client();
    
    int getFd() const { return _fd; }
    bool isAuthenticated() const { return _authenticated; }
};

#endif

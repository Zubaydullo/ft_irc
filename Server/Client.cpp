#include "Client.hpp"
#include <iostream>

Client::Client(int fd) : _fd(fd), _authenticated(false)
{
    std::cout << "Client created with FD: " << _fd << std::endl;
}

Client::~Client() {}

#include "../../../include/Server.hpp"

void Server::handelPass(int clinetFd ,  std::istringstream& iss) {
           
    std::string password;
    iss >> password;
    std::cout << "Client " << clinetFd << " sent  password " << password << std::endl;
    if(password == _password){
            std::cout << "The password correct for clinet  " << clinetFd << std::endl;
            _Client[clinetFd]->setAuthenticated(true);
    }else{
        sendToClient(clinetFd , "464 " + _Client[clinetFd]->getNickname() + " :Password incorrect");
        std::cout << "Wrong password from clinet  : "  << clinetFd<< std::endl;
        //TODO: handell the error after 
    }
}

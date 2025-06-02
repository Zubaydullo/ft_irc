#ifndef SERVER_HPP
#define  SERVER_HPP
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>
#include "Client.hpp"

class Server{ 

        private: 
            int _port;
            std::string _password;
            int  _serverSocket;
            std::vector <struct pollfd> _pollfd;
            std::map<int , Client*> _Client;
            bool _running;

        public: 
            Server(int Port , std::string& Password);      
            ~Server();

        //NOTE: i need to check if i need to implement the orthodox
           void Start();
           void Stop();
        private:
           bool createSocket();
           void acceptNewClient();
           void handleClientData(int ClinetFd);
           void removeClient(int clinetFd);
           void parseCommand(int clinetFd , const std::string& message);
           void handelPass(int clinetFd ,  std::istringstream& iss);
           void handelNick(int clinetFd , std::istringstream& iss);
           void handelUser(int clinetFd ,  std::istringstream& iss);
           void sendToClient(int clinetfd ,const  std::string& message);
            
};


#endif

#ifndef  SERVER_HPP
#define  SERVER_HPP
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cstdlib>

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <vector>
#include <map>
#include <unistd.h>
#include "Client.hpp"
#include "Channel.hpp"
#include <fstream>
class Server { 

        private: 
            int _port;
            std::string _password;
            int  _serverSocket;
            std::vector <struct pollfd> _pollfd;
            std::map<int , Client*> _Client;
            bool _running;
            std::map<std::string, std::vector<int> > _inviteList; 
            std::map<std::string , Channel*> _channels;
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
           void handleKick(int clientFd , std::istringstream& iss);
           void handleMode(int clientFd , std::istringstream& iss);
           void handleNames(int clientFd , std::istringstream& iss);
           void handleDCC(int clientFd, std::istringstream& iss);
           //Here handel the existing Users 

           // Handel the  Users && Operators 
           void handleJoin(int clientFd , std::istringstream& iss);
           void handlePart(int clientFd, std::istringstream& iss);
           void handlePrivmsg(int clientFd , std::istringstream& iss);
           void handleInvite(int clientFd , std::istringstream& iss);
           void handleTopic(int clientFd , std::istringstream& iss);
           int findClientByNick(const std::string& nickname);
        public: 
           void sendToClient(int clinetfd ,const  std::string& message);
            std::map<int, Client*>& getClients();
};


#endif

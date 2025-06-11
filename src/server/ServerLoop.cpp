#include "../../include/Server.hpp"

void Server::Start(){
        
    if(!createSocket()){
         throw std::runtime_error("Failed to create server Socket");
    }
    setupSignalHandlers();
    struct pollfd serverPoll;
    serverPoll.fd = _serverSocket;
    serverPoll.events = POLLIN;
    serverPoll.revents = 0;
    _pollfd.push_back(serverPoll);
    
    _running = true;
    std::cout  << "Server Started successfully " << std::endl;
   
    while(_running){
         
         int activity = poll(_pollfd.data(),_pollfd.size() , -1);
         if(activity == -1){
             if(_running){ 
              std::cerr << "Poll Error" << std::endl;
             }
                break;
         }
         std::cout << "Activity detected " << activity << "socket(s)" << std::endl;
        for(size_t i = 0 ; i < _pollfd.size() ;  i++){
             if(!(_pollfd[i].revents & POLLIN))
                continue;

            if(_pollfd[i].fd ==  _serverSocket){
                std::cout << "New Client trying to connect !" <<std::endl;
                 acceptNewClient();
            }else {
                std::cout << "Data from exisiting Client on fd" << std::endl;
                handleClientData(_pollfd[i].fd);
            }
            
        }
    }
}

void Server::removeClient(int ClinetFd){
    
    std::cout << "Removing client " << ClinetFd << std::endl;
    
    // Check if client exists before proceeding
    if(_Client.find(ClinetFd) == _Client.end()) {
        std::cout << "Client " << ClinetFd << " not found" << std::endl;
        return;
    }
         
    for(std::map<std::string, Channel*>::iterator channelIt = _channels.begin();
            channelIt != _channels.end(); ++channelIt) {
           
        if(channelIt->second->isMember(ClinetFd)){
            std::string nick = _Client[ClinetFd]->getNickname();
            std::string channelName = channelIt->first;
            
            std::vector<int> members = channelIt->second->getMembers();
            for(std::vector<int>::iterator it = members.begin(); it != members.end(); ++it){
                if(*it != ClinetFd){
                    sendToClient(*it, ":" + nick + " QUIT :Client disconnected");
                }
            }
            
            channelIt->second->removeOperator(ClinetFd); 
            channelIt->second->removeMember(ClinetFd);
        }
    }
    
    std::map<std::string, Channel*>::iterator it = _channels.begin(); 
    while(it != _channels.end()){ 
        if(it->second->getMemberCount() == 0){
            delete it->second;
            _channels.erase(it++);
        }else{
            ++it;
        }
    }
    
    // Fix the iterator invalidation issue
    for(std::vector<struct pollfd>::iterator pollIt = _pollfd.begin(); pollIt != _pollfd.end(); ){
        if(pollIt->fd == ClinetFd){
            pollIt = _pollfd.erase(pollIt);  // erase returns next valid iterator
            break;
        } else {
            ++pollIt;
        }
    }
    
    // Clean up client resources
    delete _Client[ClinetFd];
    _Client.erase(ClinetFd);
    close(ClinetFd);
    std::cout << "Client " << ClinetFd << " disconnected" << std::endl;
}
void Server::handleClientData(int ClinetFd){
    char buffer[1024];
    int bytesRead = recv(ClinetFd, buffer, sizeof(buffer) - 1, 0);
    if(bytesRead <= 0){
        removeClient(ClinetFd);
        return;
    }
     if(!isValidClientFd(ClinetFd)) {
        std::cerr << "Invalid client FD in handleClientData: " << ClinetFd << std::endl;
        return;
    }
    buffer[bytesRead] = '\0';
    std::string newData(buffer);
    
    // Add to client's input buffer for partial message handling
    _Client[ClinetFd]->addToInBuffer(newData);
    
    // Process complete messages (ending with \r\n or \n)
    std::string &inBuffer = _Client[ClinetFd]->getInBuffer();
    size_t pos = 0;
    
    // Look for complete messages ending with \r\n or \n
    while ((pos = inBuffer.find("\r\n")) != std::string::npos || 
           (pos = inBuffer.find("\n")) != std::string::npos) {
        
        std::string completeMessage = inBuffer.substr(0, pos);
        
        // Remove the processed message from buffer
        if (pos < inBuffer.length() - 1 && inBuffer.substr(pos, 2) == "\r\n") {
            inBuffer.erase(0, pos + 2);
        } else {
            inBuffer.erase(0, pos + 1);
        }
        
        if (!completeMessage.empty()) {
            // Clean up the message
            std::replace(completeMessage.begin(), completeMessage.end(), '\r', ' ');
            
            std::cout << "Received From Client " << ClinetFd << ": " << completeMessage << std::endl;
            parseCommand(ClinetFd, completeMessage);
        }
    }
}

#include "../../include/Dcc.hpp"
#include "../../include/server.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

DCCManager::DCCManager(Server* server):_server() , _nextPort(8000){ 

    std::cout << "DCC Manager initialized. Port range starts at : " << _nextPort << std::endl; 
}

std::string DCCManager::generateTransferId(const std::string& sender , const std::string& reciver , const std::string& fileName){
    std::ostringstream oss;
    oss << sender << "_" << reciver << "_" << fileName << "_" << time(NULL);
    
    std::string  transferId = oss.str();

    for (size_t i = 0; i < transferId.length(); ++i) {
        if (transferId[i] == ' ' || transferId[i] == '#' || transferId[i] == ':') {
            transferId[i] = '_';
        }
    }
    std::cout  <<"Generate transfer ID:" << transferId << std::endl;
    return transferId;
}


unsigned long DCCManager::ipToLong(const std::string& ip){
     
    struct in_addr addr;
    if(inet_aton(ip.c_str(),&addr) == 0){
         std::cerr << "Invalid IP address" << ip << std::endl;
         return 0;
    }
    unsigned long result = ntohl(addr.s_addr);
    std::cout << "Coverted IP  " << ip << "to long:  " << result << std::endl;
    return result;
}

std::string DCCManager::longToIp(unsigned long ip){
    struct  in_addr addr;
        addr.s_addr = htonl(ip);
        std::string  result = inet_ntoa(addr);
        std::cout << "Converted long " << ip << " to IP:  " << result << std::endl;
    return result;
}

int DCCManager::createLisentSocket(int port){
     
     int lisentSocket = socket(AF_INET,SOCK_STREAM ,0);
     if(lisentSocket == -1){ 
        std::cerr << "DCC : Failed to create socket" << std::endl;
        return -1;
     }
     int opt = 1; 
      if(setsockopt(lisentSocket , SOL_SOCKET , SO_REUSEADDR , &opt , sizeof(opt)  == -1)){
            std::cerr << "DCC : Failed to set socket options" << std::endl;
                close(lisentSocket);
            return -1;
      }
    
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = INADDR_ANY;
      addr.sin_port = htonl(port);

      if(bind(lisentSocket , (struct sockaddr*)&addr , sizeof(addr)) == -1){ 
            std::cerr << "DCC : Failed to bind to port " << port << std::endl;
            close(lisentSocket);
            return (-1);
      }
    if(listen(lisentSocket , 1)  == -1){
         std::cerr << "DCC : Failed to listen on port" << port << std::endl;
         close(lisentSocket);
         return (-1);
    }
   std::cout << "DCC: Created listen socket on port " << port << std::endl;
   return lisentSocket;
}

void DCCManager::sendDCCOffer(const std::string& sender, const std::string& receiver, const DCCTransfer& transfer){ 

        std:;std::ostringstream oss;
            oss << "DCC SEND " << transfer.fileName << " " << transfer.ip << " " << transfer.port << " " << transfer.fileSize;
        std::string dccMessage = oss.str();
        std::string privmsgCmd = ":" + sender + " PRIVMSG "+ receiver + " :" + dccMessage;
        std::map<int , Client*>& clients   = _server->getClients();
         int receiverSocket = -1;
    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second && it->second->getNickname() == receiver) {
            receiverSocket = it->first;
            break;
        }
    }
        if(receiverSocket  != -1 ){ 
            _server->sendToClient(receiverSocket , privmsgCmd);
             std::cout << "DCC: Sent offer to " << receiver << ": " << dccMessage << std::endl;
        } else {
            std::cerr << "DCC: Receiver " << receiver << " not found online" << std::endl;      
        }

}
DCCManager::~DCCManager(){ 
    for (std::map<std::string, DCCTransfer>::iterator it = _activeTransfers.begin(); 
         it != _activeTransfers.end(); ++it) {
        if (it->second.transferSocket != -1) {
            close(it->second.transferSocket);
        }
    }
    _activeTransfers.clear();
    std::cout << "DCC Manager destroyed. All transfers cleaned up." << std::endl;
}






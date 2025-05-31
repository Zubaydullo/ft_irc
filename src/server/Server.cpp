#include "../include/Server.hpp"
#include "../include/Client.hpp"



Server::Server(int port , std::string& Password):_port(port)
        ,_password(Password) ,_serverSocket(-1) , _running(false) {

            std::cout  << "Server Created with port " << _port << std::endl;
}

//---------- Socket init --------------//
bool  Server::createSocket(){
    
    int SocketFd = socket(AF_INET, SOCK_STREAM, 0);
     _serverSocket= SocketFd;

    if(SocketFd == -1){
         std::cerr << "Failed to create the socket" << std::endl;
        return false;
    }
    std::cout << "Socket Created Successfully" << std::endl;
    
        //------ start init and  bind socket --- //
    
      struct  sockaddr_in ServerAddr;
      ServerAddr.sin_family = AF_INET;
      ServerAddr.sin_port = htons( _port);
      ServerAddr.sin_addr.s_addr = INADDR_ANY;
     
       if(bind(SocketFd , (struct sockaddr* )&ServerAddr , sizeof(ServerAddr)) == -1){
           std::cerr << "Failed to bind socket to port" << _port  << std::endl;
           close(SocketFd);
           return false;
       }
        std::cout << "Bind the socket successfully" << std::endl;      
       //------- end bind socket and init adress <-----//
        
       // ---- Start listen  to the request ---- //
       
     if(listen(SocketFd, 10) == -1){
       
         std::cerr << "Failed to listen to the port : " << _port << std::endl;
        close(SocketFd);
        return false;
     }
     // ----- End Listen to the request  <-----//
     std::cout << "Server listening or connections...."  << std::endl;

    return true;
}

//----------- Socket init end -----------------//

void  Server::acceptNewClient(){
     
        struct sockaddr_in ClientAddr;
        socklen_t  ClientLen = sizeof(ClientAddr);
        int ClientFd =  accept(_serverSocket, (struct sockaddr*)&ClientAddr, &ClientLen);

         if(ClientFd == -1) {
              std::cerr <<  "Error : Accept did  work with  client"  << std::endl;
                return;
         }
         std::cout << "New Client Connected! FD:  " << ClientFd << std::endl;
         
         struct pollfd ClientPoll;
         ClientPoll.fd = ClientFd;
         ClientPoll.events = POLLIN;
        _pollfd.push_back(ClientPoll);
       _Client[ClientFd] = new Client(ClientFd); 
}

void Server::Start(){
        
    if(!createSocket()){
         throw std::runtime_error("Failed to create server Socket");
    }
    //TODO: WE  need to add the conf of the i/o non-blocking
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
              std::cerr << "Poll Error" << std::endl;
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
    
    std::cout << "Revmoving client" << ClinetFd<<  std::endl;
    close(ClinetFd);
    delete _Client[ClinetFd];
    for(size_t i = 0 ; i < _pollfd.size() ; i++){
         
        if(_pollfd[i].fd == ClinetFd){
            _pollfd.erase(_pollfd.begin() + i);
            break;
        }
    }
}
void Server::handleClientData(int ClinetFd){

    char buffer[1024];
    int bytesRead = recv(ClinetFd,buffer , sizeof(buffer) - 1 , 0);
    if(bytesRead < 0){
         
        removeClient(ClinetFd);
        return;
    }
    buffer[bytesRead] = '\0';
    std::cout << "Received from client " << ClinetFd << ": " << buffer << std::endl;
}

 
Server::~Server(){
        
        std::cout << "cnx closed" << std::endl;
}

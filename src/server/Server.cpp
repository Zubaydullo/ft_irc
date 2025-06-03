#include "../../include/Server.hpp"
#include "../../include/Client.hpp"



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
    std::string message(buffer);
    std::cout << "Received From Clinet " << ClinetFd << ": " << message << std::endl;
    parseCommand(ClinetFd , message);
}

// HACK: START  implement the  command  


void Server::parseCommand(int  clinetFd, const std::string& message){
     
    std::cout << "Parsing command from client " << clinetFd << ":" << std::endl;

    std::istringstream iss(message);
    std::string command;
    iss >> command;
    if(command == "PASS" ){
         handelPass(clinetFd , iss);
    }
    else if(command == "NICK"){ 
        handelNick(clinetFd, iss);
    }
    else if(command == "USER"){
         handelUser(clinetFd, iss);
    }else if(command == "JOIN"){
         handleJoin(clinetFd , iss);
    }else if(command == "PART"){
         handlePart(clinetFd, iss);    
    }else if(command == "PRIVMSG"){
         handlePrivmsg(clinetFd , iss);
    }
    else {
        std::cout << "Unknown  Command :  " << command   << std::endl;    
        //TODO: we neeed to handle the  unknown command here later
    }
    
}    


void Server::handlePrivmsg(int clinetFd , std::istringstream& iss) {

    std::string target , message;
    int membersFd;
    iss >> target >> message;
        
    std::getline(iss, message);

    if(!message.empty() && message[0] == ' ') message = message.substr(1);
    if(!message.empty() && message[0] == ':')  message = message.substr(1);

    std::string senderNick = _Client[clinetFd]->getNickname();
    std::cout << senderNick << " sends  to " << target << " : " << message << std::endl;
    if(target[0] == '#'){
         if(_channels.find(target) != _channels.end() ){ 
             std::vector <int> members = _channels[target]->getMembers();
             for(std::vector<int>::iterator it  = members.begin() ;  it != members.end() ; ++it){
                  
                  membersFd = *it;
                  if(membersFd != clinetFd){
                    
                 sendToClient(membersFd , ":" + senderNick + " PRIVMSG " + target + " :" + message );
                  }
             }
         }else{
              
         }
    }
    //TODO :  We need to add User to User later 
}
void Server::handleJoin(int clientFd , std::istringstream& iss){
      
    std::string channelName;
    iss >> channelName;

    if(!_Client[clientFd]->isRegistered()){
         
        sendToClient(clientFd , "451 :You have not registered");
        return;
    }

        std::cout << "Client  " << clientFd << " Wants to jion channel: " << channelName << std::endl; 
    
        if(_channels.find(channelName) == _channels.end()){  // if both equal to end channel don't exisiting 
        
         _channels[channelName] = new Channel(channelName);
         std::cout << "Created new channel : " << channelName << std::endl;
    }   
    _channels[channelName]->addMember(clientFd);
    //TODO  add the user to user 
    std::string nick = _Client[clientFd]->getNickname();
    sendToClient(clientFd , ":" + nick + " JOIN " + channelName);

    // now we  need to  send the info to the channel 
    sendToClient(clientFd , "353 " + nick + " = " + channelName + " : " + nick);
    sendToClient(clientFd , "366 " + nick + " " + channelName + " :End of /NAMES  list");
}

void Server::handlePart(int clientFd, std::istringstream& iss){


    std::string channelName;
    iss >> channelName;

    if(_channels.find(channelName) != _channels.end()){ 
        _channels[channelName]->removeMember(clientFd);
        std::string nick = _Client[clientFd]->getNickname();
        sendToClient(clientFd , " : " + nick + " PART " + channelName);

        std::cout << "Client " << clientFd << " left channel: " << channelName << std::endl;

    }

}
void Server::handelPass(int clinetFd ,  std::istringstream& iss) {
           
    std::string password;
    iss >> password;
    std::cout << "Client " << clinetFd << " sent  password " << password << std::endl;
    if(password == _password){
            std::cout << "The password correct for clinet  " << clinetFd << std::endl;
            _Client[clinetFd]->setAuthenticated(true);
    }else{

        std::cout << "Wrong password from clinet  : "  << clinetFd<< std::endl;
        //TODO: handell the error after 
    }
}
void Server::sendToClient(int clientFd,const  std::string& message){
        
    std::string msg = message + "\r\n";
    send(clientFd ,msg.c_str(), msg.length(),0 );
    std::cout <<  "Sent to the Client " << clientFd << " : " << message << std::endl; 
}
void Server::handelNick(int clientFd, std::istringstream& iss){ 
        std::string nickName;
    iss >> nickName;
        std::cout << "Clinet  :"  << clientFd << " Wants  nickname:" << nickName << std::endl;
        
      _Client[clientFd]->setNickname(nickName);     
 
}

void Server::handelUser(int clinetFd , std::istringstream& iss){
     
     std::string username, hostname , servername, realname;
     iss  >> username >> hostname >> servername;
     std::getline(iss, realname);

    std::cout << "ClinedFd" << clinetFd << " user info : " << username << std::endl;
        
    _Client[clinetFd]->setUsername(username);
    _Client[clinetFd]->setRealname(realname);
    if(_Client[clinetFd]->isAuthenticated() && !_Client[clinetFd]->getNickname().empty()){

        _Client[clinetFd]->setRegistered(true);
        std::cout << "ClinetFd " << clinetFd << " is now fully registered "  << std::endl;
        sendToClient(clinetFd , "001 -> " + _Client[clinetFd]->getNickname() + " :Welcome to the IRC Server!" );    
        sendToClient(clinetFd , "002 -> " + _Client[clinetFd]->getNickname() + " :Your host is localhost" );    
        sendToClient(clinetFd , "003 -> " + _Client[clinetFd]->getNickname() + " :Server created recently" );    
    }
}



Server::~Server(){
        
        std::cout << "cnx closed" << std::endl;
}

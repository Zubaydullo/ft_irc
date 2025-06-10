#include "../../include/Server.hpp"
#include "../../include/Client.hpp"



Server::Server(int port , std::string& Password):_port(port)
        ,_password(Password) ,_serverSocket(-1) , _running(false) {

            std::cout  << "Server Created with port " << _port << std::endl;
    _dccManager = new DCCManager(this);
    std::cout << "DCC Manager initialized" << std::endl;
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
      ServerAddr.sin_port = htons(_port);
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

DCCManager*  Server::getDCCManager(){ 
    return _dccManager;
}
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
    //TODO: WE  need to add the conf of the i/o non-blockingi
    // fcntl 
    struct pollfd serverPoll;
    serverPoll.fd = _serverSocket;
    serverPoll.events = POLLIN;
    serverPoll.revents = 0;
    _pollfd.push_back(serverPoll);
    
    _running = true;
    std::cout  << "Server Started successfully " << std::endl;
   
    while(_running){
          if (_dccManager) {
            _dccManager->processTransfers();
        }
         int activity = poll(_pollfd.data(),_pollfd.size() , 100);
         if(activity == -1){
              std::cerr << "Poll Error" << std::endl;
                break;
         }
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
         
    for(std::map<std::string , Channel*>::iterator channelIt = _channels.begin();
            channelIt != _channels.end(); ++channelIt) {
           
        if(channelIt->second->isMember(ClinetFd)){
            std::string nick  = _Client[ClinetFd]->getNickname();
            std::string channelName = channelIt->first;

        std::vector<int> members = channelIt->second->getMembers();
        for(std::vector<int>::iterator  it = members.begin() ; it != members.begin(); ++it){
             if(*it != ClinetFd){
                 sendToClient(*it,":" + nick + " QUIT :Client disconnected");
             }
            channelIt->second->removeOperator(ClinetFd); 
            channelIt->second->removeMember(ClinetFd);
        }
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

    for(std::vector<struct pollfd>::iterator pollIt = _pollfd.begin(); pollIt != _pollfd.end(); ++pollIt ){
        if(pollIt->fd == ClinetFd){
            _pollfd.erase(pollIt);
            break;
        }

    }
}
std::map<int,Client*>& Server::getClients(){ 
        
    return _Client;
}
void Server::handleClientData(int ClinetFd){

    char buffer[1024];
    int bytesRead = recv(ClinetFd,buffer , sizeof(buffer) - 1 , 0);
    if(bytesRead <= 0){
         
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
        std::string originalMessage = message;
        if(originalMessage.find("\001DCC") != std::string::npos){
              handleDCC(clinetFd , iss);
        }else {    
        handlePrivmsg(clinetFd , iss);
        }
    }else if(command == "DCC"){
               handleDCC(clinetFd , iss);
    }else if(command == "DCCACCEPT"){
            std::string transferId;
         iss >> transferId;
         handleDCCAccept(clinetFd, transferId);
    }else if(command =="DCCREJECT"){
                std::string transferId;
         iss >> transferId;
         handleDCCReject(clinetFd, transferId);
    }else if(command == "SHOWDCC"){
    if (_dccManager) {
        std::cout << "=== Active Transfers Debug ===" << std::endl;
        sendToClient(clinetFd, ":Server NOTICE " + _Client[clinetFd]->getNickname() + " :=== Active DCC Transfers ===");
        
        // Access _activeTransfers directly through DCCManager
        
        // You'll need to add this simple function to DCCManager
        _dccManager->showActiveTransfers(clinetFd);
        
    } else {
        sendToClient(clinetFd, ":Server NOTICE " + _Client[clinetFd]->getNickname() + " :DCC Manager not initialized");
    }
    }else if(command == "KICK"){
         handleKick(clinetFd , iss); 
    }else if(command == "MODE"){
        //TODO: implement the mode 
         handleMode(clinetFd , iss);
    }else if(command == "INVITE"){
         handleInvite(clinetFd , iss);
    }else if(command == "TOPIC"){
         handleTopic(clinetFd , iss);
    }else {
        std::cout << "Unknown  Command :  " << command   << std::endl;    
        //TODO: we neeed to handle the  unknown command here later
    }
    
}    
void Server::handleDCC(int clientFd, std::istringstream& iss) {
    std::string firstParam;
    iss >> firstParam;
    
    std::string senderNick = _Client[clientFd]->getNickname();
    
    // Check if this is a direct DCC command (like "DCC SEND zabdirak test.txt")
    if (firstParam == "SEND") {
        // Direct DCC command format: DCC SEND target filename
        std::string target, fileName;
        iss >> target >> fileName;
        
        std::cout << "Direct DCC SEND request from " << senderNick << " to " << target 
                  << " file: " << fileName << std::endl;
        
        int targetFd = findClientByNick(target);
        if (targetFd == -1) {
            sendToClient(clientFd, "401 " + senderNick + " " + target + " :No such nick");
            return;
        }
        
        // Create the full file path
        std::string fullPath = "./" + fileName;
        
        if (_dccManager) {
            bool success = _dccManager->initiateDCCSend(senderNick, target, fullPath);
            if (!success) {
                sendToClient(clientFd, ":Server NOTICE " + senderNick + " :Failed to initiate DCC transfer - file not found: " + fileName);
            }
        }
        return;
    }
    
    // Otherwise, this is a PRIVMSG DCC format: target \001DCC SEND filename ip port size\001
    std::string target = firstParam;
    std::string restOfMessage;
    std::getline(iss, restOfMessage);
    
    // Parse DCC message from PRIVMSG
    size_t start = restOfMessage.find("\001DCC");
    if (start != std::string::npos) {
        start += 4; // Skip "\001DCC"
        size_t end = restOfMessage.find("\001", start);
        if (end != std::string::npos) {
            restOfMessage = restOfMessage.substr(start, end - start);
        } else {
            restOfMessage = restOfMessage.substr(start);
        }
    }
    
    std::istringstream dccIss(restOfMessage);
    std::string dccCommand, fileName, ip, port, fileSize;
    dccIss >> dccCommand;
    
    if (dccCommand == "SEND") {
        dccIss >> fileName >> ip >> port >> fileSize;
        
        std::cout << "PRIVMSG DCC SEND request from " << senderNick << " to " << target 
                  << " file: " << fileName << std::endl;
        
        int targetFd = findClientByNick(target);
        if (targetFd == -1) {
            sendToClient(clientFd, "401 " + senderNick + " " + target + " :No such nick");
            return;
        }
        
        // Create the full file path
        std::string fullPath = "./" + fileName;
        
        if (_dccManager) {
            bool success = _dccManager->initiateDCCSend(senderNick, target, fullPath);
            if (!success) {
                sendToClient(clientFd, ":Server NOTICE " + senderNick + " :Failed to initiate DCC transfer - file not found: " + fileName);
            }
        }
    }
    else if (dccCommand == "ACCEPT") {
        std::string transferId;
        dccIss >> transferId;
        handleDCCAccept(clientFd, transferId);
    }
    else if (dccCommand == "REJECT") {
        std::string transferId;
        dccIss >> transferId;
        handleDCCReject(clientFd, transferId);
    }
    else {
        std::cout << "Unknown DCC command: " << dccCommand << std::endl;
        sendToClient(clientFd, "421 " + senderNick + " " + dccCommand + " :Unknown command");
    }
}
void Server::handleDCCAccept(int clientFd, const std::string& transferId) {
    std::string receiverNick = _Client[clientFd]->getNickname();
    
    if (_dccManager) {
        bool success = _dccManager->acceptDCCTransfer(receiverNick, transferId);
        if (!success) {
            sendToClient(clientFd, ":Server NOTICE " + receiverNick + " :Failed to accept DCC transfer " + transferId);
        }
    }
}

void Server::handleDCCReject(int clientFd, const std::string& transferId) {
    std::string receiverNick = _Client[clientFd]->getNickname();
    
    if (_dccManager) {
        bool success = _dccManager->rejectDCCTransfer(receiverNick, transferId);
        if (!success) {
            sendToClient(clientFd, ":Server NOTICE " + receiverNick + " :Failed to reject DCC transfer " + transferId);
        }
    }
}
int Server::findClientByNick(const std::string& nickname){
     
    for(std::map<int , Client*>::iterator it = _Client.begin() ; it != _Client.end(); ++it  ){
            
    if(it->second->getNickname() == nickname) return it->first;
    }
    return (-1);
}
void Server::handleInvite(int clientFd , std::istringstream& iss){
     std::string targetNick , channelName;
     iss >> targetNick >> channelName;
     
     if(targetNick.empty() || channelName.empty()){ 
               sendToClient(clientFd, "461 " + _Client[clientFd]->getNickname() + " INVITE :Not enough parameters");       
         return;
     }
     if(_channels.find(channelName) == _channels.end()){
             sendToClient(clientFd, "403 " + _Client[clientFd]->getNickname() + " " + channelName + " :No such channel");
         return ;
     }
     if(!_channels[channelName]->isMember(clientFd)){
            sendToClient(clientFd, "442 " + _Client[clientFd]->getNickname() + " " + channelName + " :You're not on that channel"); 
         return;
     }
     if(_channels[channelName]->isInviteOnly() && !_channels[channelName]->isOperator(clientFd)){
              sendToClient(clientFd, "482 " + _Client[clientFd]->getNickname() + " " + channelName + " :You're not channel operator");
         return;
     }
    int targetFd = findClientByNick(targetNick);
    if(targetFd == -1){ 
         sendToClient(clientFd, "401 " + _Client[clientFd]->getNickname() + " " + targetNick + " :No such nick");
        return;
    }
    if(_channels[channelName]->isMember(targetFd)){
          sendToClient(clientFd, "443 " + _Client[clientFd]->getNickname() + " " + targetNick + " " + channelName + " :is already on channel");
        return;
    }
    _inviteList[channelName].push_back(targetFd);

    std::string inviterNick = _Client[clientFd]->getNickname();
    sendToClient(targetFd, ":" + inviterNick + " INVITE " + targetNick + " " + channelName);
    sendToClient(clientFd, "341 " + inviterNick + " " + targetNick + " " + channelName);
 std::cout << inviterNick << " invited " << targetNick << " to " + channelName << std::endl;
    
}
void  Server::handleMode(int clientFd , std::istringstream& iss){
    std::string target, modeString,param;
    iss >> target >> modeString >> param;

    if(target[0] != '#'){
         sendToClient(clientFd , "501 " + _Client[clientFd]->getNickname() + " :Unknown MODE flag ");
         return ;
    }
    if(_channels.find(target) == _channels.end()){
         sendToClient(clientFd , "403 " + _Client[clientFd]->getNickname() + " " + target + " :No such channel ");
         return;
    }
    if(!_channels[target]->isOperator(clientFd)){ 
        sendToClient(clientFd ,"482 " + _Client[clientFd]->getNickname() + " " + target + " :You're not channel operator" );
        return;
    }
        std::string nick = _Client[clientFd]->getNickname();
      
             std::vector<int> members = _channels[target]->getMembers();
   if(modeString == "+o"){
         int targetFd = findClientByNick(param);
         if(targetFd == -1){
              sendToClient(clientFd, "401 " + nick + " " + param + " :No such nick");
              return ;
         }

         if(!_channels[target]->isMember(targetFd)){
             sendToClient(clientFd , "441 " + nick + " " + param + " " + target + " :They aren't on that channel"); 
            return;
         }
         _channels[target]->addOperator(targetFd);
         for(std::vector<int>::iterator it = members.begin(); it != members.end(); ++it){
            sendToClient(*it, ":" + nick + " MODE " + target + " +o " + param);   
         }
         std::cout << nick << " gave operator status to " << param << " in " << target << std::endl;
    }else if(modeString == "-o"){
         int targetFd = findClientByNick(param);
               if(targetFd == -1){
                    
              sendToClient(clientFd, "401 " + nick + " " + param + " :No such nick");
                    return ;
               }

        _channels[target]->removeOperator(clientFd);
         std::vector<int> members = _channels[target]->getMembers();
        for (std::vector<int>::iterator it = members.begin() ; it != members.end() ; ++it){ 
            sendToClient(*it, ":" + nick + " MODE " + target + " -o " + param);
        }
  std::cout << nick << " removed operator status from " << param << " in " << target << std::endl;
    }else if (modeString == "+i") {
        _channels[target]->setInviteOnly(true);
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " +i");
        }
    }else if (modeString == "-i") {
        _channels[target]->setInviteOnly(false);
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " -i");
        }
    }else if (modeString == "+t") {
        _channels[target]->setTopicRestricted(true);
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " +t");
        }
    }else if (modeString == "-t") {
        _channels[target]->setTopicRestricted(false);
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " -t");
        }
    }else if (modeString == "+k") {
        if (param.empty()) {
            sendToClient(clientFd, "461 " + nick + " MODE :Not enough parameters");
            return;
        }
        _channels[target]->setPassword(param);
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " +k " + param);
        }
    }else if (modeString == "-k") {
        _channels[target]->setPassword("");
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " -k");
        }
    }else if (modeString == "+l") {
        if (param.empty()) {
            sendToClient(clientFd, "461 " + nick + " MODE :Not enough parameters");
            return;
        }
        int limit = atoi(param.c_str());
        if (limit <= 0) {
            sendToClient(clientFd, "472 " + nick + " " + param + " :Invalid limit");
            return;
        }
        _channels[target]->setUserLimit(limit);
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " +l " + param);
        }
    }else if (modeString == "-l") {
        _channels[target]->setUserLimit(0);
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " -l");
        }
    }else {
        sendToClient(clientFd, "472 " + nick + " " + modeString + " :Unknown mode char to me");
    }
}
void Server::handleKick(int clientFd , std::istringstream& iss){ 
    std::string channelName, targetNick, reason;
    iss >> channelName >> targetNick;
    std::getline(iss, reason);

    if (!reason.empty() && reason[0] == ' ') reason = reason.substr(1);
    if (!reason.empty() && reason[0] == ':') reason = reason.substr(1);
    if (reason.empty()) reason = "No reason given";

    if(_channels.find(channelName) == _channels.end()){
         sendToClient(clientFd , "403 " + _Client[clientFd]->getNickname()  + 
                 channelName + ":No such channel ");
        return;
    }
    if(!_channels[channelName]->isMember(clientFd))
        {
             sendToClient(clientFd,"442 " + _Client[clientFd]->getNickname() + " " + channelName + "You're not that  channel " );
            return;
        }
     if(!_channels[channelName]->isOperator(clientFd)){ 
           sendToClient(clientFd, "482 " + _Client[clientFd]->getNickname() + channelName + "You are not the channel operator");
        return;
     } 
     int targetFd = -1;
    for (std::map<int, Client*>::iterator it = _Client.begin(); it != _Client.end(); ++it) {
        if (it->second->getNickname() == targetNick) {
            targetFd = it->first;
            break;
        }
    }
    if(targetFd == -1 ){ 
        sendToClient(clientFd , "441 " + _Client[clientFd]->getNickname() 
                + " " + targetNick + " " + channelName + "The aren't on the that channel" );
    }
    std::string kickerNick = _Client[clientFd]->getNickname(); 
    std::vector<int> members = _channels[channelName]->getMembers();
    for(std::vector<int>::iterator it = members.begin() ; it != members.end() ; ++it){
         sendToClient(*it, ":" + kickerNick + " KICK " + channelName + " " + targetNick + " :" + reason);
    }
    _channels[channelName]->removeMember(targetFd);
    std::cout <<kickerNick  <<   " kicked" << targetNick << "from " << channelName  << " ( " << reason << ")" <<  std::endl;
}
void Server::handleTopic(int clientFd , std::istringstream& iss){
   std::string channelName, topic;
    iss >> channelName;
    std::getline(iss,topic);
    
    std::cout << "DEBUG: The value of topic is " << topic << std::endl; 
    if (!topic.empty() && topic[0] == ' ') topic = topic.substr(1);
    if (!topic.empty() && topic[0] == ':') topic = topic.substr(1);
      
    std::cout << "DEBUG: The value of topic is after feltring : " << topic << std::endl; 
    
    if (_channels.find(channelName) == _channels.end()) {
        sendToClient(clientFd, "403 " + _Client[clientFd]->getNickname() + " " + channelName + " :No such channel");
        return;
    }
    
    if (!_channels[channelName]->isMember(clientFd)) {
        sendToClient(clientFd, "442 " + _Client[clientFd]->getNickname() + " " + channelName + " :You're not on that channel");
        return;
    }
    std::string nick = _Client[clientFd]->getNickname();
    std::cout << "DEBUG : This the length of the topic :  " << topic.length() << std::endl;
    std::cout << "DEBUD : this t the empby value " << (topic.empty() ? true : false) << std::endl;
    if(topic.length() <= 1){
        std::string currentTopic = _channels[channelName]->getTopic();
        if (currentTopic.empty()) {
            sendToClient(clientFd, "331 " + nick + " " + channelName + " :No topic is set");
        } else {
            sendToClient(clientFd, "332 " + nick + " " + channelName + " :" + currentTopic);
        }
    }else {
        if(_channels[channelName]->isTopicRestricted() && !_channels[channelName]->isOperator(clientFd)){
               sendToClient(clientFd, "482 " + nick + " " + channelName + " :You're not channel operator");
            return;
        } 
        std::cout << "enter here to check  after change the topic " << std::endl;
        
      _channels[channelName]->setTopic(topic);
      
      std::vector<int> members = _channels[channelName]->getMembers();
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " TOPIC " + channelName + " :" + topic);
        }
        
        std::cout << nick << " changed topic of " << channelName << " to: " << topic << std::endl;
    
    }
}
void Server::handlePrivmsg(int clinetFd , std::istringstream& iss) {

    std::string target , message;
    iss >> target;
        
    std::getline(iss, message);

    if(!message.empty() && message[0] == ' ') message = message.substr(1);
    if(!message.empty() && message[0] == ':')  message = message.substr(1);

    std::string senderNick = _Client[clinetFd]->getNickname();
    std::cout << senderNick << " sends  to " << target << " : " << message << std::endl;
    if(target[0] == '#'){


             std::vector <int> members = _channels[target]->getMembers();
             Channel * channel= _channels[target];

             
             for(std::vector<int>::iterator it  = members.begin() ;  it != members.end() ; ++it){
                  
                   int membersFd = *it;
                   
                  if((membersFd != clinetFd  && channel->isMember(clinetFd))){
                    
                 sendToClient(membersFd , ":" + senderNick + " PRIVMSG " + target + " :" + message );
                  }else{ 
                     sendToClient(clinetFd , "Your are not a Member on the channel : " + target);
                     std::cout << "The Client Fd " << clinetFd << " won't be able to send the "   <<  
                          " \n message becasue he is not a member" << std::endl; 
                  }
             }
         
    }else{
                for(std::map<int , Client*>::iterator it = _Client.begin() ; it != _Client.end() ; ++it){
                     
                    if(it->second->getNickname() == target){
                         sendToClient(it->first , ":" + senderNick + " PRIVMSG "+ target + " : " + message );
                         std::cout << "Private message send from " << senderNick << " to " << target << std::endl;
                         return ;
                    }

                }

                sendToClient(clinetFd , "401 " + senderNick  + " " + target + " No: such nick/channel");
         }
    
}

void Server::handleJoin(int clientFd , std::istringstream& iss){
      
    std::string channelName,password;
    iss >> channelName >> password;

    if(!_Client[clientFd]->isRegistered()){
         
        sendToClient(clientFd , "451 :You have not registered");
        return;
    }

        std::cout << "Client  " << clientFd << " Wants to jion channel: " << channelName << std::endl; 
    
        if(_channels.find(channelName) == _channels.end()){  // if both equal to end channel don't exisiting 
        
         _channels[channelName] = new Channel(channelName);
         std::cout << "Created new channel : " << channelName << std::endl;
    }
        Channel * channel= _channels[channelName];

    if(channel->isMember(clientFd)){
          sendToClient(clientFd, "443 " + _Client[clientFd]->getNickname() + " " + channelName + " :is already on channel");
        return;
    }
    if(channel->isInviteOnly()){
        bool isInvited = false;
        std::vector<int>& invites = _inviteList[channelName];
        for(std::vector<int>::iterator it = invites.begin() ; it != invites.end(); ++it){
            if(*it == clientFd){ 
                isInvited = true;
                invites.erase(it);
                break;
            }
        }
        if(!isInvited){
         sendToClient(clientFd, "473 " + _Client[clientFd]->getNickname() + " " + channelName + " :Cannot join channel (+i)");
        return;    
        }
    }
    if(!channel->getPassword().empty() && password != channel->getPassword()){
            sendToClient(clientFd, "475 " + _Client[clientFd]->getNickname() + " " + channelName + " :Cannot join channel (+k)");
        return; 
    }
    if(channel->getUserLimit() > 0 && channel->getMemberCount() >= channel->getUserLimit()){
         sendToClient(clientFd, "471 " + _Client[clientFd]->getNickname() + " " + channelName + " :Cannot join channel (+l)");
        return;
    }

        // after passs all the checkes
    
      bool isFirstMember = (_channels[channelName]->getMemberCount() == 0);
    _channels[channelName]->addMember(clientFd);
     if(isFirstMember){
         _channels[channelName]->addOperator(clientFd);
          std::cout << "Client " << clientFd << " is now operator of " << channelName << std::endl;
    }
    std::string nick = _Client[clientFd]->getNickname();
    std::vector<int> members = _channels[channelName]->getMembers();
    for(std::vector<int>::iterator it = members.begin() ; it != members.end() ; ++it){

        if(*it != clientFd) sendToClient(*it , ":" + nick +  " JOIN " + channelName);
         
    }
    sendToClient(clientFd , ":" + nick + " JOIN " + channelName);
    
    // now we  need to  send the info to the channel 
    sendToClient(clientFd , "353 " + nick + " = " + channelName + " : " + nick);
    sendToClient(clientFd , "366 " + nick + " " + channelName + " :End of /NAMES  list");
}

void Server::handlePart(int clientFd, std::istringstream& iss){


    std::string channelName;
    iss >> channelName;

    if(_channels.find(channelName) != _channels.end()){ 
     
        std::string nick = _Client[clientFd]->getNickname();
        
        std::vector<int> members = _channels[channelName]->getMembers();
        for(std::vector<int>::iterator  it = members.begin() ; it != members.end(); ++ it){ 
            sendToClient(*it , " : " + nick + " PART " + channelName);
            
        }
        _channels[channelName]->removeMember(clientFd);
        std::cout << "Client " << clientFd << " left channel: " << channelName << std::endl;

        if(_channels[channelName]->getMemberCount() == 0){
             delete _channels[channelName];
             _channels.erase(channelName);
             std::cout << "Deleted empty channel:  " << channelName << std::endl;
        }
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
        
    if (_dccManager) {
        delete _dccManager;
        _dccManager = NULL;
    }
    
    // Clean up clients
    for (std::map<int, Client*>::iterator it = _Client.begin(); it != _Client.end(); ++it) {
        if (it->second) {
            delete it->second;
        }
    }
    _Client.clear();
    
    // Clean up channels
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        if (it->second) {
            delete it->second;
        }
    }
    _channels.clear();
    
    if (_serverSocket != -1) {
        close(_serverSocket);
    }
    
    std::cout << "Server destroyed and resources cleaned up" << std::endl;
}

#include "../../../include/Server.hpp"

void Server::handlePrivmsg(int clinetFd , std::istringstream& iss) {

    std::string target , message;
    iss >> target;
        
    std::getline(iss, message);

    if(!message.empty() && message[0] == ' ') message = message.substr(1);
    if(!message.empty() && message[0] == ':')  message = message.substr(1);
   if (message.find("DCC SEND") != std::string::npos || message.find("\001DCC SEND") != std::string::npos) {
        std::istringstream dccStream(target + " " + message);
        handleDCC(clinetFd, dccStream);
        return;
    }

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

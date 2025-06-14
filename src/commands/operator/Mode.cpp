#include "../../../include/Server.hpp"


void Server::handleMode(int clientFd, std::istringstream& iss) {
    std::string target, modeString, param;
    iss >> target >> modeString >> param;
    
    std::string nick = _Client[clientFd]->getNickname();
    
    if (target[0] != '#') {
        if (target == nick) {
            if (modeString.empty()) {
                sendToClient(clientFd, "221 " + nick + " :+");
                return;
            }
            
            if (modeString == "+i") {
                sendToClient(clientFd, ":" + nick + " MODE " + nick + " :+i");
                return;
            } else if (modeString == "-i") {
                sendToClient(clientFd, ":" + nick + " MODE " + nick + " :-i");
                return;
            } else if (modeString[0] == '+' || modeString[0] == '-') {
                sendToClient(clientFd, ":" + nick + " MODE " + nick + " :" + modeString);
                return;
            }
        } else {
            sendToClient(clientFd, "502 " + nick + " :Cannot change mode for other users");
            return;
        }
        
        sendToClient(clientFd, "501 " + nick + " :Unknown MODE flag");
        return;
    }
    
    if (_channels.find(target) == _channels.end()) {
        sendToClient(clientFd, "403 " + nick + " " + target + " :No such channel");
        return;
    }
    
    if (!_channels[target]->isOperator(clientFd)) { 
        sendToClient(clientFd, "482 " + nick + " " + target + " :You're not channel operator");
        return;
    }
    
    std::vector<int> members = _channels[target]->getMembers();
    
    if (modeString == "+o") {
        int targetFd = findClientByNick(param);
        if (targetFd == -1) {
            sendToClient(clientFd, "401 " + nick + " " + param + " :No such nick");
            return;
        }
        
        if (!_channels[target]->isMember(targetFd)) {
            sendToClient(clientFd, "441 " + nick + " " + param + " " + target + " :They aren't on that channel"); 
            return;
        }
        
        _channels[target]->addOperator(targetFd);
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " +o " + param);   
        }
        std::cout << nick << " gave operator status to " << param << " in " << target << std::endl;
        
    } else if (modeString == "-o") {
        int targetFd = findClientByNick(param);
        if (targetFd == -1) {
            sendToClient(clientFd, "401 " + nick + " " + param + " :No such nick");
            return;
        }
        
        if (!_channels[target]->isMember(targetFd)) {
            sendToClient(clientFd, "441 " + nick + " " + param + " " + target + " :They aren't on that channel"); 
            return;
        }
        
        _channels[target]->removeOperator(targetFd);  
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) { 
            sendToClient(*it, ":" + nick + " MODE " + target + " -o " + param);
        }
        std::cout << nick << " removed operator status from " << param << " in " << target << std::endl;
        
    } else if (modeString == "+i") {
        _channels[target]->setInviteOnly(true);
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " +i");
        }
        
    } else if (modeString == "-i") {
        _channels[target]->setInviteOnly(false);
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " -i");
        }
        
    } else if (modeString == "+t") {
        _channels[target]->setTopicRestricted(true);
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " +t");
        }
        
    } else if (modeString == "-t") {
        _channels[target]->setTopicRestricted(false);
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " -t");
        }
        
    } else if (modeString == "+k") {
        if (param.empty()) {
            sendToClient(clientFd, "461 " + nick + " MODE :Not enough parameters");
            return;
        }
        _channels[target]->setPassword(param);
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " +k " + param);
        }
        
    } else if (modeString == "-k") {
        _channels[target]->setPassword("");
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " -k");
        }
        
    } else if (modeString == "+l") {
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
        
    } else if (modeString == "-l") {
        _channels[target]->setUserLimit(0);
        for (std::vector<int>::iterator it = members.begin(); it != members.end(); ++it) {
            sendToClient(*it, ":" + nick + " MODE " + target + " -l");
        }
        
    } else {
        sendToClient(clientFd, "472 " + nick + " " + modeString + " :Unknown mode char to me");
    }
}

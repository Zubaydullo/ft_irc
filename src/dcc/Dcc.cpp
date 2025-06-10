#include "../../include/Dcc.hpp"
#include "../../include/Server.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

DCCManager::DCCManager(Server* server):_server(server) , _nextPort(8000){ 

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


bool DCCManager::createFileReceiver(const std::string& transferId) {
    std::map<std::string, DCCTransfer>::iterator it = _activeTransfers.find(transferId);
    if (it == _activeTransfers.end()) {
        return false;
    }
    
    DCCTransfer& transfer = it->second;
    
    // Fork a process to handle file reception
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process - handles file reception
        receiveFileProcess(transfer, transferId);
        exit(0);
    } else if (pid > 0) {
        // Parent process - continue normally
        std::cout << "DCC: Started file receiver process for " << transferId << std::endl;
        return true;
    } else {
        std::cerr << "DCC: Failed to fork receiver process" << std::endl;
        return false;
    }
}

// The actual file receiving logic
void DCCManager::receiveFileProcess(const DCCTransfer& transfer, const std::string& transferId) {
    std::cout << "DCC: Receiver process starting for " << transfer.fileName << transferId << std::endl;
    
    // Wait a moment for sender to be ready
    usleep(500000); // 0.5 seconds
    
    // Create socket to connect to sender
    int receiverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (receiverSocket == -1) {
        std::cerr << "DCC: Failed to create receiver socket" << std::endl;
        return;
    }
    
    // Connect to sender
    struct sockaddr_in senderAddr;
    senderAddr.sin_family = AF_INET;
    senderAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    senderAddr.sin_port = htons(transfer.port);
    
    int attempts = 0;
    while (attempts < 10) {
        if (connect(receiverSocket, (struct sockaddr*)&senderAddr, sizeof(senderAddr)) == 0) {
            break;
        }
        attempts++;
        usleep(100000); // Wait 0.1 seconds between attempts
    }
    
    if (attempts >= 10) {
        std::cerr << "DCC: Failed to connect to sender after 10 attempts" << std::endl;
        close(receiverSocket);
        return;
    }
    
    std::cout << "DCC: Connected to sender successfully!" << std::endl;
    
    // Create output directory and file
    system("mkdir -p received_files");
    std::string outputPath = "received_files/" + transfer.fileName;
    
    std::ofstream outFile(outputPath.c_str(), std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "DCC: Cannot create output file: " << outputPath << std::endl;
        close(receiverSocket);
        return;
    }
    
    // Receive the file
    const int CHUNK_SIZE = 1024;
    char buffer[CHUNK_SIZE];
    unsigned long totalReceived = 0;
    
    std::cout << "DCC: Starting file reception..." << std::endl;
    
    while (totalReceived < transfer.fileSize) {
        int bytesReceived = recv(receiverSocket, buffer, CHUNK_SIZE, 0);
        if (bytesReceived <= 0) {
            break;
        }
        
        outFile.write(buffer, bytesReceived);
        totalReceived += bytesReceived;
        
        // Show progress every 1KB
        if (totalReceived % 1024 == 0 || totalReceived >= transfer.fileSize) {
            double progress = (double)totalReceived / transfer.fileSize * 100.0;
            std::cout << "DCC: Receive progress: " << std::fixed << std::setprecision(1) 
                      << progress << "% (" << totalReceived << "/" << transfer.fileSize << " bytes)" << std::endl;
        }
    }
    
    outFile.close();
    close(receiverSocket);
    
    if (totalReceived >= transfer.fileSize) {
        std::cout << "DCC: File received successfully!" << std::endl;
        std::cout << "DCC: File saved to: " << outputPath << std::endl;
        
        // Create a completion marker file
        std::string markerPath = outputPath + ".complete";
        std::ofstream marker(markerPath.c_str());
        marker << "Transfer completed successfully" << std::endl;
        marker.close();
        
        std::cout << "DCC: Reception complete - check " << outputPath << std::endl;
    } else {
        std::cerr << "DCC: File transfer incomplete (" << totalReceived << "/" << transfer.fileSize << " bytes)" << std::endl;
    }
}

bool DCCManager::acceptDCCTransfer(const std::string& receiver, const std::string& transferId) {
    std::cout << "DCC: " << receiver << " accepting transfer " << transferId << std::endl;
    
    std::map<std::string, DCCTransfer>::iterator it = _activeTransfers.find(transferId);
    if (it == _activeTransfers.end()) {
        std::cerr << "DCC: Transfer ID not found: " << transferId << std::endl;
        return false;
    }
    
    DCCTransfer& transfer = it->second;
    
    if (transfer.reciver != receiver) {
        std::cerr << "DCC: Wrong receiver for transfer " << transferId << std::endl;
        return false;
    }
    
    if (transfer.isActive) {
        std::cerr << "DCC: Transfer " << transferId << " already in progress" << std::endl;
        return false;
    }
    
    // Mark transfer as accepted
    transfer.isActive = true;
    transfer.bytesTransferred = 0;
    
    // Start the file receiver process
    if (createFileReceiver(transferId)) {
        std::cout << "DCC: File receiver started for " << transfer.fileName << std::endl;
        sendDCCAccept(transfer.sender, receiver, transferId);
        return true;
    } else {
        std::cerr << "DCC: Failed to start file receiver" << std::endl;
        return false;
    }
}
void DCCManager::showActiveTransfers(int clientFd) {
    std::cout << "=== Showing Active Transfers ===" << std::endl;
    
    if (_activeTransfers.empty()) {
        _server->sendToClient(clientFd, ":Server NOTICE user :No active DCC transfers");
        std::cout << "No active transfers" << std::endl;
        return;
    }
    
    for (std::map<std::string, DCCTransfer>::iterator it = _activeTransfers.begin(); 
         it != _activeTransfers.end(); ++it) {
        
        std::string transferId = it->first;
        DCCTransfer& transfer = it->second;
        
        std::string msg = ":Server NOTICE user :Transfer ID: " + transferId;
        _server->sendToClient(clientFd, msg);
        
        std::cout << "Active Transfer: [" << transferId << "] - " 
                  << transfer.sender << " -> " << transfer.reciver 
                  << " (" << transfer.fileName << ")" << std::endl;
    }
}

bool DCCManager::rejectDCCTransfer(const std::string& receiver, const std::string& transferId) {
    std::cout << "DCC: " << receiver << " rejecting transfer " << transferId << std::endl;
    
    std::map<std::string, DCCTransfer>::iterator it = _activeTransfers.find(transferId);
    if (it == _activeTransfers.end()) {
        std::cerr << "DCC: Transfer ID not found: " << transferId << std::endl;
        return false;
    }
    
    DCCTransfer& transfer = it->second;
    
    std::string rejectMsg = ":" + receiver + " NOTICE " + transfer.sender + " :DCC SEND " + transfer.fileName + " rejected";
    int senderSocket = _server->findClientByNick(transfer.sender);
    if (senderSocket != -1) {
        _server->sendToClient(senderSocket, rejectMsg);
    }
    
    if (transfer.transferSocket != -1) {
        close(transfer.transferSocket);
    }
    
    _activeTransfers.erase(it);
    std::cout << "DCC: Transfer " << transferId << " rejected and cleaned up" << std::endl;
    
    return true;
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
      if((setsockopt(lisentSocket , SOL_SOCKET , SO_REUSEADDR , &opt , sizeof(opt))  == -1)){
            std::cerr << "DCC : Failed to set socket options" << std::endl;
                close(lisentSocket);
            return -1;
      }
    
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = INADDR_ANY;
      addr.sin_port = htons(port);

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

        std::ostringstream oss;
            oss << "DCC SEND " << transfer.fileName << " " << transfer.ip << " " << transfer.port << " " << transfer.fileSize;
        std::string dccMessage = oss.str();
        std::string privmsgCmd = ":" + sender + " PRIVMSG "+ receiver + " :\001" + dccMessage + "\001";
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

bool DCCManager::initiateDCCSend(const std::string& sender, const std::string& receiver, const std::string& filepath){ 

    std::cout << "DCC: " << sender << " wants to send " << filepath << " to " << receiver << std::endl;

    std::ifstream file(filepath.c_str() , std::ios::binary | std::ios::ate);
    if(!file.is_open()){ 
        std::cerr << "DCC: file not found: " <<filepath << std::endl;
        return false;
    }

    unsigned long fileSize = file.tellg();
    file.close();
    if(fileSize == 0){
         std::cerr << "DCC: File is emppty: " << filepath << std::endl;
         return false;
    }
    std::string filename = filepath;
    size_t lastSlash = filepath.find_last_of("/\\");
    if(lastSlash != std::string::npos){
         filename = filepath.substr(lastSlash +1);
    }

    int port = _nextPort++;
    if(_nextPort > 9000){ 
        _nextPort = 8000;
    }
    int lisentSocket =  createLisentSocket(port);
    if(lisentSocket == -1){ 
        std::cerr << "DCC: Failed to create listen socket on port" << port << std::endl;
        return false;
    }

     // --- ini the struct --------/
    DCCTransfer transfer;

    transfer.fileName = filename;
    transfer.sender = sender;
    transfer.reciver = receiver;
    transfer.fileSize = fileSize;
    transfer.ip = ipToLong("127.0.0.1");
    transfer.port = port;
    transfer.transferSocket =  lisentSocket;
    transfer.isActive = false;
    transfer.bytesTransferred = 0;
    // --- end init the struct ----/
    std::string transferId = generateTransferId(sender , receiver , filename);
    _activeTransfers[transferId] = transfer;

    sendDCCOffer(sender , receiver , transfer);
std::cout << "DCC: Transfer " << transferId << " initiated. Waiting for receiver to accept..." << std::endl;
    return true;
}
 void DCCManager::sendDCCAccept(const std::string& sender, const std::string& receiver, const std::string& transferId){ 
    
    std::string acceptMsg = ":" + receiver + " NOTICE " + sender + " :DCC SEND accepted - Transfer ID: " + transferId + " starting";
    
    int senderSocket = _server->findClientByNick(sender);
    if (senderSocket != -1) {
        _server->sendToClient(senderSocket, acceptMsg);
        std::cout << "DCC: Notified " << sender << " that " << receiver << " accepted transfer " << transferId << std::endl;
    } else {
        std::cerr << "DCC: Failed to find sender " << sender << " for transfer acceptance notification" << std::endl;
    }
    
    int receiverSocket = _server->findClientByNick(receiver);
    if (receiverSocket != -1) {
        std::string receiverMsg = ":Server NOTICE " + receiver + " :DCC transfer " + transferId + " accepted. Starting file transfer...";
        _server->sendToClient(receiverSocket, receiverMsg);
    }
}  
void DCCManager::notifyTransferFailed(const DCCTransfer& transfer, const std::string& reason) {
    std::cout << "DCC: Transfer failed - " << reason << std::endl;
    // notfiy here the sender 
    std::string senderMsg = ":" + transfer.reciver + " NOTICE " + transfer.sender + " :DCC SEND failed: " + reason;
    int senderSocket = _server->findClientByNick(transfer.sender);
    if (senderSocket != -1) {
        _server->sendToClient(senderSocket, senderMsg);
    }
   // notify here the reciver 
    std::string receiverMsg = ":" + transfer.sender + " NOTICE " + transfer.reciver + " :DCC SEND failed: " + reason;
    int receiverSocket = _server->findClientByNick(transfer.reciver);
    if (receiverSocket != -1) {
       _server->sendToClient(receiverSocket, receiverMsg);
    }
}

void DCCManager::handleDCCData(int socket) {
    DCCTransfer* transfer = NULL;
    std::string transferId = "";
    
    // Find the transfer
    for(std::map<std::string, DCCTransfer>::iterator it = _activeTransfers.begin(); 
        it != _activeTransfers.end(); ++it) { 
        if(it->second.transferSocket == socket && it->second.isActive) {
            transfer = &(it->second);
            transferId = it->first;
            break;
        }
    }
    
    if (!transfer) {
        std::cerr << "DCC: No active transfer found for socket " << socket << std::endl;
        return;
    }
    
    std::cout << "DCC: Sending file data for " << transfer->fileName << std::endl;
    std::string fullPath = "./" + transfer->fileName;
    std::ifstream file(fullPath.c_str(), std::ios::binary);

    if(!file.is_open()) { 
        std::cerr << "DCC: Cannot open file for reading: " << fullPath << std::endl;
        notifyTransferFailed(*transfer, "File not accessible");
        cleanupTransfer(transferId);
        return;
    }
    
    file.seekg(transfer->bytesTransferred);
    const int CHUNK_SIZE = 1024;
    char buffer[CHUNK_SIZE];
    
    while(transfer->bytesTransferred < transfer->fileSize) {
        // Re-validate transfer still exists (in case it was cleaned up)
        std::map<std::string, DCCTransfer>::iterator checkIt = _activeTransfers.find(transferId);
        if (checkIt == _activeTransfers.end()) {
            std::cout << "DCC: Transfer was cleaned up during processing" << std::endl;
            file.close();
            return;
        }
        transfer = &(checkIt->second); // Update pointer in case map was modified
        
        int toRead = CHUNK_SIZE;
        if(transfer->bytesTransferred + CHUNK_SIZE > transfer->fileSize) {
            toRead = transfer->fileSize - transfer->bytesTransferred;
        }
    
        file.read(buffer, toRead);
        int bytesRead = file.gcount();
        
        if (bytesRead <= 0) {
            std::cerr << "DCC: Error reading file" << std::endl;
            notifyTransferFailed(*transfer, "File read error");
            file.close();
            cleanupTransfer(transferId);
            return;
        } 
        
        int bytesSent = send(socket, buffer, bytesRead, 0);
        if (bytesSent <= 0) {
            std::cerr << "DCC: Error sending data over socket" << std::endl;
            notifyTransferFailed(*transfer, "Network error");
            file.close();
            cleanupTransfer(transferId);
            return;
        }
        
        transfer->bytesTransferred += bytesSent;
        
        // Progress reporting
        if (transfer->bytesTransferred % 10240 == 0 || transfer->bytesTransferred == transfer->fileSize) {
            double progress = (double)transfer->bytesTransferred / transfer->fileSize * 100.0;
            std::cout << "DCC: Transfer progress: " << progress << "% (" 
                      << transfer->bytesTransferred << "/" << transfer->fileSize << " bytes)" << std::endl;
        }
        
        if (transfer->bytesTransferred >= transfer->fileSize) {
            std::cout << "DCC: File transfer completed successfully!" << std::endl;
            notifyTransferComplete(*transfer);
            file.close();
            cleanupTransfer(transferId);
            return;
        }
        
        // Avoid overwhelming the network
        usleep(1000);
    }
    
    file.close();
}


void DCCManager::notifyTransferComplete(const DCCTransfer& transfer){ 

        std::cout << "DCC: Transfer completed - " << transfer.fileName << std::endl;
        std::string senderMsg = ":Server NOTICE " + transfer.sender + " :DCC SEND completed: " + transfer.fileName;
    int senderSocket = _server->findClientByNick(transfer.sender);
    if (senderSocket != -1) {
        _server->sendToClient(senderSocket, senderMsg);
    }
    std::string receiverMsg = ":Server NOTICE " + transfer.reciver + " :DCC SEND completed: " + transfer.fileName;
    int receiverSocket = _server->findClientByNick(transfer.reciver);
    if (receiverSocket != -1) {
        _server->sendToClient(receiverSocket, receiverMsg);
    }

}
void DCCManager::cleanupTransfer(const std::string& transferId) {
    std::map<std::string, DCCTransfer>::iterator it = _activeTransfers.find(transferId);
    if (it == _activeTransfers.end()) {
        std::cout << "DCC: Transfer " << transferId << " already cleaned up" << std::endl;
        return;
    }
    
    DCCTransfer& transfer = it->second;
    
    if (transfer.transferSocket != -1) {
        std::cout << "DCC: Closing socket " << transfer.transferSocket << std::endl;
        close(transfer.transferSocket);
        transfer.transferSocket = -1; // Prevent double-close
    }
    
    std::cout << "DCC: Erasing transfer " << transferId << std::endl;
    _activeTransfers.erase(it);
    
    std::cout << "DCC: Transfer " << transferId << " cleaned up successfully" << std::endl;
}

void DCCManager::handleDCCReceive(int socket) {
    DCCTransfer* transfer = NULL;
    std::string transferId = "";
    
    for(std::map<std::string, DCCTransfer>::iterator it = _activeTransfers.begin(); 
        it != _activeTransfers.end(); ++it) { 
        if(it->second.transferSocket == socket && it->second.isActive) {
            transfer = &(it->second);
            transferId = it->first;
            break;
        }
    }
    
    if (!transfer) {
        std::cerr << "DCC: No active transfer found for receiving socket " << socket << std::endl;
        return;
    }
    
    std::cout << "DCC: Receiving file data for " << transfer->fileName << std::endl;
    
    system("mkdir -p received_files");
    
    std::string outputPath = "received_files/" + transfer->fileName;
    std::ofstream outFile(outputPath.c_str(), std::ios::binary | std::ios::app);
    
    if (!outFile.is_open()) {
        std::cerr << "DCC: Cannot create output file: " << outputPath << std::endl;
        notifyTransferFailed(*transfer, "Cannot create output file");
        cleanupTransfer(transferId);
        return;
    }
    
    const int CHUNK_SIZE = 1024;
    char buffer[CHUNK_SIZE];
    
    int bytesReceived = recv(socket, buffer, CHUNK_SIZE, 0);
    if (bytesReceived <= 0) {
        if (bytesReceived == 0) {
            std::cout << "DCC: Transfer completed - received " << transfer->bytesTransferred << " bytes" << std::endl;
            std::cout << "DCC: File saved to: " << outputPath << std::endl;
            notifyTransferComplete(*transfer);
        } else {
            std::cerr << "DCC: Error receiving data" << std::endl;
            notifyTransferFailed(*transfer, "Network error during receive");
        }
        outFile.close();
        cleanupTransfer(transferId);
        return;
    }
    
    outFile.write(buffer, bytesReceived);
    transfer->bytesTransferred += bytesReceived;
    
    if (transfer->bytesTransferred % 10240 == 0 || transfer->bytesTransferred >= transfer->fileSize) {
        double progress = (double)transfer->bytesTransferred / transfer->fileSize * 100.0;
        std::cout << "DCC: Receive progress: " << progress << "% (" 
                  << transfer->bytesTransferred << "/" << transfer->fileSize << " bytes)" << std::endl;
    }
    
    if (transfer->bytesTransferred >= transfer->fileSize) {
        std::cout << "DCC: File reception completed successfully!" << std::endl;
        std::cout << "DCC: File saved to: " << outputPath << std::endl;
        outFile.close();
        notifyTransferComplete(*transfer);
        cleanupTransfer(transferId);
        return;
    }
    
    outFile.close();
}
void DCCManager::processTransfers() {
    if (_activeTransfers.empty()) {
        return;
    }
    
    std::vector<std::string> transfersToRemove;
    
    for(std::map<std::string, DCCTransfer>::iterator it = _activeTransfers.begin(); 
        it != _activeTransfers.end(); ++it) { 
            
        DCCTransfer& transfer = it->second;
        std::string transferId = it->first;
        
        if(transfer.isActive && transfer.transferSocket != -1) {
            struct pollfd pfd;
            pfd.fd = transfer.transferSocket;
            pfd.events = POLLOUT | POLLIN;  
            pfd.revents = 0;
            
            int pollResult = poll(&pfd, 1, 0);
            
            if(pollResult > 0) {
                if (pfd.revents & POLLOUT) {
                    handleDCCData(transfer.transferSocket);
                    break;
                } else if (pfd.revents & POLLIN) {
                    handleDCCReceive(transfer.transferSocket);
                    break;
                }
            }
            else if(pollResult < 0) { 
                std::cerr << "DCC: Poll error on socket " << transfer.transferSocket << std::endl;
                notifyTransferFailed(transfer, "Socket error");
                transfersToRemove.push_back(transferId);
            }
        }
    }
    
    for(std::vector<std::string>::iterator it = transfersToRemove.begin(); 
        it != transfersToRemove.end(); ++it) {
        cleanupTransfer(*it);
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






#ifndef  DCC_HPP 
#define  DCC_HPP



#include <string>
#include <map>
#include <vector>






class Server;

struct DCCTransfer{ 
        std::string fileName;
        std::string sender;
        std::string reciver;
        unsigned long fileSize;
        unsigned long ip;
        int port;
        int transferSocket;
        bool isActive;
        unsigned long bytesTransferred;

        DCCTransfer() : fileSize(0) , ip(0) , transferSocket(-1) ,isActive(false) , bytesTransferred(0){}
};

class DCCManager{ 
    

        private:
            Server* _server;
            std::map<std::string, DCCTransfer>_activeTransfers;
            int _nextPort;
        // Helper Function 
         std::string generateTransferId(const std::string& sender , const std::string& reciver , const std::string& fileName);
         unsigned long ipToLong(const std::string& ip);
         std::string longToIp(unsigned long ip);
         int createLisentSocket(int port);
         void sendDCCOffer(const std::string& sender ,const std::string& reciver , const DCCTransfer& transfer);
         void sendDCCAffer(const std::string& sender ,const std::string& reciver , const DCCTransfer& transferId);
         void notifyTransferComplete(const DCCTransfer& transfer);
         void notifyTransferFailed(const DCCTransfer& transfer , const std::string& reason);
        public:
         DCCManager(Server* server);
        ~DCCManager();

        bool initiateDCCSend(const std::string& sender , const std::string& receiver , const std::string& filepath);
        bool acceptDCCTransfer(const std::string& reciver , const std::string& trasferId);
        bool rejectDCCTransfer(const std::string& reciver , const std::string& trasferId);
      //Trasfer  managment
      
       void handleDCCData(int socket);
       void cancelTransfer(const std::string& transferId);
       void cleanupTransfer(const std::string& transferId);

       //status function
       std::vector<std::string> getActiveTransfer() const;
       DCCTransfer* getTransfer(const std::string& transferId);

       // Server interface 
       void processTransfers();
        
};


class DCCParser{ 
    public:
        static bool isDCCComand(const std::string& message);
        static std::string getDCCType(const std::string& message);
        static std::vector<std::string> parseDCCParams(const std::string& message);
};

#endif

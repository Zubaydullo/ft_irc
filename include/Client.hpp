#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <vector>
#include <sstream>

class Client {
private:
    int sockfd;
    std::string server;
    int port;
    std::string nickname;
    std::string username;
    std::string realname;
    std::string clientIP;     
    std::string _outBuffer;
    bool connected;
    std::string _inBuffer;
    bool authenticated; 
    bool registered;
    
    // Private methods
    bool createSocket();
    bool connectToServer();
    void parseMessage(const std::string& message);
    Client(const Client& other);
    Client& operator=(const Client& other);
public:
    // Constructors & Destructor
    Client(int fd);  // Constructor for server-side client
    Client(const std::string& server, int port);  // Constructor for client-side connection
    ~Client();

    // Connection methods
    bool connect();
    void disconnect();
    bool isConnected() const;

    // Authentication methods
    void setNickname(const std::string& nick);
    void setUsername(const std::string& user);
    void setRealname(const std::string& real);
    void authenticate();

    // Communication methods
    void sendRaw(const std::string& message);
    std::string receiveMessage();
    void processMessages();

    // Getters and setters for server use
    std::string getClientIP() const;
    int getFd() const;
    bool isAuthenticated() const;
    bool isRegistered() const;
    void setAuthenticated(bool auth);
    void setRegistered(bool reg);
    std::string getNickname();
    std::string getUsername();
    std::string getRealname();
    void addToInBuffer(const std::string& data);
    std::string& getInBuffer();
    void setClientIP(const std::string& ip);     
    void addToOutBuffer(const std::string& msg);   
    std::string& getOutBuffer();
};

#endif

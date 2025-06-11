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
    //NOTE: haithem  add modificatin  ------------/
    std::string _inBuffer;
    bool authenticated; 
    bool registered;
    //NOTE: haithem modification end ------/
    // Private methods
    bool createSocket();
    bool connectToServer();
    void parseMessage(const std::string& message);
    std::vector<std::string> split(const std::string& str, char delimiter);

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

    // IRC Commands
    void join(const std::string& channel);
    void part(const std::string& channel);
    void privmsg(const std::string& target, const std::string& message);
    void quit(const std::string& message = "Goodbye");
    void pong(const std::string& server);

    // Utility methods
    void handlePing(const std::string& server);
    void displayMessage(const std::string& message);
    //NOTE: haithem public add On //
    std::string getClientIP() const;
    int getFd() const { return sockfd; }
    bool isAuthenticated() const { return authenticated; }
    bool isRegistered() const { return registered; }
    void setAuthenticated(bool auth) { authenticated = auth; }
    void setRegistered(bool reg) { registered = reg; }
    std::string  getNickname() { return nickname;}
    std::string  getUsername() { return username;}
    std::string  getRealname() { return realname;}
    void addToInBuffer(const std::string& data) { _inBuffer += data; }
    std::string& getInBuffer() { return _inBuffer; }
    void setClientIP(const std::string& ip);     
    void addToOutBuffer(const std::string& msg);   
    std::string& getOutBuffer();
    //NOTE: haithem finish public add on //
};

#endif

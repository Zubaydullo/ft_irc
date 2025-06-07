#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <sstream>
#include <ctime>
#include <set>

class Client {
private:
    // Socket and connection info
    int _fd;
    std::string _server;
    int _port;
    std::string _nickname;
    std::string _username;
    std::string _realname;
    std::string _hostname;
    std::string _servername;
    bool _authenticated;
    bool _registered;
    std::time_t _lastActivity;
    std::set<std::string> _channels;
    bool _pingSent;
    std::time_t _pingTime;
    
    // Private methods
    bool createSocket();
    bool connectToServer(const std::string& host, int port);
    void parseMessage(const std::string& message);
    std::vector<std::string> split(const std::string& str, char delimiter);
    bool validateMessage(const std::string& message);
    void handleError(const std::string& error);
    void handleStateChange(const std::string& state);
    void handleConnectionError();
    bool validateNickname(const std::string& nickname);
    bool validateChannelName(const std::string& channel);

public:
    // Constructors & Destructor
    Client(int fd);  // Constructor for server-side client
    Client(const std::string& server, int port);  // Constructor for client-side connection
    ~Client();

    // Connection methods
    bool connect();
    void disconnect();
    bool isConnected() const;
    void updateActivity();

    // Authentication methods
    bool setNickname(const std::string& nickname);
    bool setUsername(const std::string& username);
    bool setRealname(const std::string& realname);
    bool setHostname(const std::string& hostname);
    bool setServername(const std::string& servername);
    bool authenticate();
    bool isAuthenticated() const;
    bool isRegistered() const;
    void setAuthenticated(bool authenticated);
    void setRegistered(bool registered);

    // Channel management
    bool joinChannel(const std::string& channel);
    bool partChannel(const std::string& channel);
    bool setChannelMode(const std::string& channel, const std::string& mode);
    bool setTopic(const std::string& channel, const std::string& topic);
    bool kickUser(const std::string& channel, const std::string& nickname, const std::string& reason);
    bool inviteUser(const std::string& channel, const std::string& nickname);
    bool isInChannel(const std::string& channel) const;
    std::set<std::string> getChannels() const;

    // Communication methods
    bool sendRaw(const std::string& message);
    std::string receiveMessage();
    void processMessages();
    bool sendMessage(const std::string& target, const std::string& message);
    bool sendNotice(const std::string& target, const std::string& message);
    bool ping(const std::string& server);
    bool pong(const std::string& server);
    bool who(const std::string& target);
    bool list(const std::string& channel);
    bool quit(const std::string& reason);

    // IRC Commands
    bool join(const std::string& channel);
    bool part(const std::string& channel);
    bool privmsg(const std::string& target, const std::string& message);
    bool notice(const std::string& target, const std::string& message);
    bool mode(const std::string& target, const std::string& mode);
    bool topic(const std::string& channel, const std::string& topic);
    bool kick(const std::string& channel, const std::string& user, const std::string& reason);
    bool invite(const std::string& user, const std::string& channel);

    // Utility methods
    void displayMessage(const std::string& message);
    int getFd() const;
    std::string getNickname() const;
    std::string getUsername() const;
    std::string getRealname() const;
    std::string getHostname() const;
    std::string getServername() const;
    std::time_t getLastActivity() const;
    bool isPingSent() const;
    std::time_t getPingTime() const;

    // Setters
    void setPingSent(bool pingSent);
    void setPingTime(std::time_t pingTime);
};

#endif

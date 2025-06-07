#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>
#include <ctime>
#include "Client.hpp"
#include "Channel.hpp"

class Server {
private:
    // Server configuration
    int _port;
    std::string _password;
    std::string _serverName;
    std::string _serverVersion;
    std::string _serverCreationDate;
    size_t _maxClients;
    size_t _maxChannels;
    
    // Server state
    int _serverSocket;
    bool _running;
    std::time_t _startTime;
    
    // Client management
    std::vector<struct pollfd> _pollfd;
    std::map<int, Client*> _clients;
    std::map<std::string, Client*> _nicknameMap;
    
    // Channel management
    std::map<std::string, Channel*> _channels;
    
    // Private methods
    bool createSocket();
    void acceptNewClient();
    void handleClientData(int clientFd);
    void removeClient(int clientFd);
    void handleClientDisconnection(int clientFd);
    void handleClientTimeout(int clientFd);
    void handleError(const std::string& error);
    
    // Command handlers
    void parseCommand(int clientFd, const std::string& message);
    void handlePass(int clientFd, std::istringstream& iss);
    void handleNick(int clientFd, std::istringstream& iss);
    void handleUser(int clientFd, std::istringstream& iss);
    void handleJoin(int clientFd, std::istringstream& iss);
    void handlePart(int clientFd, std::istringstream& iss);
    void handlePrivmsg(int clientFd, std::istringstream& iss);
    void handleNotice(int clientFd, std::istringstream& iss);
    void handleMode(int clientFd, std::istringstream& iss);
    void handleTopic(int clientFd, std::istringstream& iss);
    void handleKick(int clientFd, std::istringstream& iss);
    void handleInvite(int clientFd, std::istringstream& iss);
    void handleQuit(int clientFd, std::istringstream& iss);
    void handlePing(int clientFd, std::istringstream& iss);
    void handlePong(int clientFd, std::istringstream& iss);
    void handleWho(int clientFd, std::istringstream& iss);
    void handleList(int clientFd, std::istringstream& iss);
    
    // Utility methods
    void sendToClient(int clientFd, const std::string& message);
    void broadcastToChannel(const std::string& channel, const std::string& message, int excludeFd = -1);
    bool validateNickname(const std::string& nickname);
    bool validateChannelName(const std::string& channel);
    void checkClientTimeout();
    void cleanupChannels();
    void sendWelcomeMessages(int clientFd);
    std::string getChannelMembers(const std::string& channel);

public:
    // Constructors & Destructor
    Server(int port, const std::string& password);
    ~Server();

    // Server control
    void start();
    void stop();
    bool isRunning() const;
    
    // Server information
    std::string getServerName() const;
    std::string getServerVersion() const;
    std::string getServerCreationDate() const;
    size_t getMaxClients() const;
    size_t getMaxChannels() const;
    size_t getClientCount() const;
    size_t getChannelCount() const;
    std::time_t getUptime() const;
    
    // Client management
    Client* getClient(int clientFd);
    Client* getClientByNickname(const std::string& nickname);
    std::vector<Client*> getClients() const;
    
    // Channel management
    Channel* getChannel(const std::string& channelName);
    std::vector<Channel*> getChannels() const;
    void createChannel(const std::string& channelName, Client* creator);
    void removeChannel(const std::string& channelName);
};

#endif

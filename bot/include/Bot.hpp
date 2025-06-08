#ifndef BOT_HPP
#define BOT_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <errno.h>

class Bot {
private:
    int _socket;
    std::string _server;
    int _port;
    std::string _password;
    std::string _nickname;
    std::string _username;
    std::string _realname;
    bool _connected;
    bool _authenticated;
    std::vector<std::string> _jokes;
    
    void initializeJokes();
    void sendMessage(const std::string& message);
    void handleMessage(const std::string& message);
    void handlePrivMsg(const std::string& sender, const std::string& target, const std::string& message);
    void handleCommand(const std::string& sender, const std::string& target, const std::string& command, const std::string& args);
    void sendPrivMsg(const std::string& target, const std::string& message);
    void joinChannel(const std::string& channel);
    std::string getRandomJoke();
    std::vector<std::string> split(const std::string& str, char delimiter);

public:
    Bot(const std::string& server, int port, const std::string& password);
    ~Bot();
    
    bool connect();
    void authenticate();
    void run();
    void disconnect();
};

#endif 
#pragma once
#include <netinet/in.h>
#include <string>

class Server
{
public:
    Server(int port);
    ~Server();
    void run();

private:
    int _port;
    int _serverSocket;
    void handleClient(int clientSocket, sockaddr_in clientAddr);
    void BroadcastMessage(std::string message, int senderSocket);
};
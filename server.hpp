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
    void broadcastMessage(std::string message, int senderSocket);
    void handleMessage(
        std::string username,
        std::string incomingData,
        int firstDelimiterIndex,
        int secondDelimiterIndex,
        char *buffer,
        int bufferSize,
        int clientSocket);
};
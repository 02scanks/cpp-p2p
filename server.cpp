#include "server.hpp"
#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <fstream>
#include <unistd.h>
#include <algorithm>
#include <mutex>

struct Client
{
    int _clientSocket;
};

std::vector<Client> _connectedPeers;
std::mutex _peerMutex;

Server::Server(int port) : _port(port)
{
    // create server socket
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket < 0)
    {
        throw std::runtime_error("TCP Socket Failure");
    }

    // create address
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(_port);

    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        throw std::runtime_error("setsockopt failed");
    }

    // bind
    int bindStatus = bind(_serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (bindStatus < 0)
    {
        throw std::runtime_error("Binding Failure");
    }

    // set server to listening
    int listenStatus = listen(_serverSocket, 5);
    if (listenStatus < 0)
    {
        throw std::runtime_error("Listening Failure");
    }

    std::cout << "Listening....\n";
}

Server::~Server()
{
    close(_serverSocket);
}

void Server::run()
{
    // loop to accept client connections
    while (true)
    {
        sockaddr_in clientAddr{};
        socklen_t clientSize = sizeof(clientAddr);
        int clientSocket = accept(_serverSocket, (struct sockaddr *)&clientAddr, &clientSize);

        std::thread clientThread(&Server::handleClient, this, clientSocket, clientAddr);
        clientThread.detach();

        // create new client to add to connectedPeers list
        Client newClient;
        newClient._clientSocket = clientSocket;

        {
            std::lock_guard<std::mutex> lock(_peerMutex);
            _connectedPeers.push_back(newClient);
        }

        std::cout << "Client: " << inet_ntoa(clientAddr.sin_addr) << " Connected\n";
    }
}

void Server::BroadcastMessage(std::string message, int senderSocket)
{
    {
        std::lock_guard<std::mutex> lock(_peerMutex);
        for (size_t i = 0; i < _connectedPeers.size(); i++)
        {
            if (_connectedPeers[i]._clientSocket == senderSocket)
                continue;

            send(_connectedPeers[i]._clientSocket, message.c_str(), message.size(), 0);
        }
    }
}

void Server::handleClient(int clientSocket, sockaddr_in clientAddr)
{
    // message buffer
    char buffer[1024];

    // grab ip once
    std::string clientIP = inet_ntoa(clientAddr.sin_addr);

    while (true)
    {
        ssize_t chunk = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (chunk <= 0)
        {
            std::cout << "User: " << clientIP << " Disconnected\n";

            // close connection
            close(clientSocket);

            {
                std::lock_guard<std::mutex> lock(_peerMutex);

                // remove from peer list
                _connectedPeers.erase(
                    std::remove_if(_connectedPeers.begin(), _connectedPeers.end(),
                                   [clientSocket](const Client &c)
                                   {
                                       return c._clientSocket == clientSocket;
                                   }),
                    _connectedPeers.end());
            }

            break;
        }

        buffer[chunk] = '\0';

        // append sender ip to broadcast to others
        std::string broadcastStr = clientIP + ": " + buffer;

        BroadcastMessage(broadcastStr, clientSocket);

        std::cout << clientIP << ": " << buffer << std::endl;
    }
}

int main()
{
    try
    {
        Server server(8080);
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal Server Error: " << e.what() << "\n";
        return 1;
    }
}
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
    std::string _username;
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

#pragma region FIX THIS
        // get username from inital chunk
        char buffer[1024];
        ssize_t chunk = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (chunk <= 0)
        {
            throw std::runtime_error("Failed to recieve inital user chunk containg username");
            return;
        }
#pragma endregion

        buffer[chunk] = '\0';

        std::thread clientThread(&Server::handleClient, this, clientSocket, clientAddr);
        clientThread.detach();

        // create new client to add to connectedPeers list
        Client newClient;
        newClient._clientSocket = clientSocket;
        newClient._username = buffer;

        {
            std::lock_guard<std::mutex> lock(_peerMutex);
            _connectedPeers.push_back(newClient);
        }

        std::cout << "Client: " << newClient._username << " Connected\n";
    }
}

void Server::broadcastMessage(std::string message, int senderSocket)
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

    // grab username once
    std::string username;
    {
        std::lock_guard<std::mutex> lock(_peerMutex);
        for (const auto &client : _connectedPeers)
        {
            if (client._clientSocket == clientSocket)
            {
                username = client._username;
                break;
            }
        }
    }

    while (true)
    {

        std::string incomingData;
        int firstDelimiterIndex;
        int secondDelimiterIndex;

        // loop to build header contents
        while (true)
        {
            ssize_t chunk = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

            // users disconnected
            if (chunk <= 0)
            {
                std::cout << "User: " << username << " Disconnected\n";

                // close connection
                close(clientSocket);

                // remove client from peerlist
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

                return;
            }

            buffer[chunk] = '\0';
            incomingData.append(buffer);

            firstDelimiterIndex = incomingData.find("|");
            secondDelimiterIndex = incomingData.find("|", firstDelimiterIndex + 1);

            // check that we got the header
            if (secondDelimiterIndex != std::string::npos)
            {
                break;
            }
        }

        // check that we actually got a correct header
        if (firstDelimiterIndex == std::string::npos || secondDelimiterIndex == std::string::npos)
        {
            throw std::runtime_error("Fatal header parsing error");
            return;
        }

        // extract header segments
        std::string payloadType = incomingData.substr(0, firstDelimiterIndex);

        if (payloadType == "MSG")
        {
            handleMessage(username, incomingData, firstDelimiterIndex, secondDelimiterIndex, buffer, sizeof(buffer), clientSocket);
        }
    }
}

void Server::handleMessage(
    std::string username,
    std::string incomingData,
    int firstDelimiterIndex,
    int secondDelimiterIndex,
    char buffer[],
    int bufferSize,
    int clientSocket)
{
    std::string payloadLengthStr = incomingData.substr(firstDelimiterIndex + 1, secondDelimiterIndex - firstDelimiterIndex - 1);
    int payloadLength = std::stoi(payloadLengthStr);

    // check if header already contains payload
    std::string payload = incomingData.substr(secondDelimiterIndex + 1);

    // loop again to get any missing payload data
    while (payload.length() < payloadLength)
    {
        ssize_t chunk = recv(clientSocket, buffer, bufferSize, 0);
        if (chunk <= 0)
            break;

        buffer[chunk] = '\0';
        payload.append(buffer);
    }

    // append senders username to broadcast to others
    std::string broadcastStr = username + ": " + payload;

    broadcastMessage(broadcastStr, clientSocket);

    std::cout << username << ": " << payload << std::endl;
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
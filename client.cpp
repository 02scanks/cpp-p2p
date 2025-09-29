#include "client.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <fstream>
#include <unistd.h>
#include <string>

void Client::receiveLoop()
{
    char buffer[1024];
    while (true)
    {
        ssize_t chunk = recv(_clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (chunk <= 0)
            break;

        buffer[chunk] = '\0';
        std::cout << buffer << std::endl;
    }
}
Client::~Client()
{
    close(_clientSocket);
}

void Client::run()
{
    // create client socket
    _clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_clientSocket < 0)
    {
        throw std::runtime_error("TCP Socket Failure");
    }

    // create address
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // hard coded connecting IP for now

    if (connect(_clientSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        throw std::runtime_error("Connection Failure");
    }

    std::cout << "Enter a username for this session\n";
    std::string username;
    std::cin >> username;

    send(_clientSocket, username.c_str(), username.size(), 0);

    std::cout << "Connection Succesful\n";

    // create client thread
    std::thread receivingThread(&Client::receiveLoop, this);
    receivingThread.detach();

    std::string input;
    while (std::getline(std::cin, input))
    {
        send(_clientSocket, input.c_str(), input.size(), 0);
    }

    close(_clientSocket);
}

int main()
{
    try
    {
        Client client;
        client.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal Client Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
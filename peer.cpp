#include "peer.hpp"
#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <algorithm>
#include <mutex>
#include <string>
#include <algorithm>

int main()
{
    Peer peer;

    std::cout << "\033[34m" << R"(
/$$   /$$ /$$   /$$ /$$       /$$        /$$$$$$  /$$                   /$$    
| $$$ | $$| $$  | $$| $$      | $$       /$$__  $$| $$                  | $$    
| $$$$| $$| $$  | $$| $$      | $$      | $$  \__/| $$$$$$$   /$$$$$$  /$$$$$$  
| $$ $$ $$| $$  | $$| $$      | $$      | $$      | $$__  $$ |____  $$|_  $$_/  
| $$  $$$$| $$  | $$| $$      | $$      | $$      | $$  \ $$  /$$$$$$$  | $$    
| $$\  $$$| $$  | $$| $$      | $$      | $$    $$| $$  | $$ /$$__  $$  | $$ /$$
| $$ \  $$|  $$$$$$/| $$$$$$$$| $$$$$$$$|  $$$$$$/| $$  | $$|  $$$$$$$  |  $$$$/
|__/  \__/ \______/ |________/|________/ \______/ |__/  |__/ \_______/   \___/  

)"

              << std::endl;

    std::cout << "Enter a username for this session\n";
    std::string username;
    std::cin >> username;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    peer.setUsername(username);

    std::cout << "Enter a port to listen on\n";
    int port;
    std::cin >> port;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Connect to peer? (y/n): " << std::endl;
    std::string choice;
    std::getline(std::cin, choice);
    if (choice == "y")
    {
        std::cout << "Enter peer IP: ";
        std::string ip;
        std::getline(std::cin, ip);

        std::cout << "Enter peer port: ";
        int peerPort;
        std::cin >> peerPort;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        peer.connectToPeer(ip, peerPort, username);
    }

    std::thread listeningThread(&Peer::startListening, &peer, port);
    listeningThread.detach();

    peer.userInputLoop();

    return 0;
}

void Peer::setUsername(std::string username)
{
    Peer::_username = username;
}

void Peer::startListening(int port)
{
    std::cout << "Listening Initialized" << std::endl;

    _listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_listeningSocket < 0)
    {
        throw std::runtime_error("Failed to create listening socket");
    }

    sockaddr_in listeninAddr{};
    listeninAddr.sin_family = AF_INET;
    listeninAddr.sin_addr.s_addr = INADDR_ANY;
    listeninAddr.sin_port = htons(port);

    int opt = 1;
    if (setsockopt(_listeningSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        throw std::runtime_error("setsockopt failed");
    }

    int bindStatus = bind(_listeningSocket, (struct sockaddr *)&listeninAddr, sizeof(listeninAddr));
    if (bindStatus < 0)
    {
        throw std::runtime_error("Failed to bind to listening socket");
    }

    // start listening
    int listenStatus = listen(_listeningSocket, 5);
    if (listenStatus < 0)
    {
        throw std::runtime_error("Failed to start listening");
    }

    std::cout << "Awaiting Connections...." << std::endl;

    while (true)
    {
        sockaddr_in peerAddr{};
        socklen_t peerSize = sizeof(peerAddr);
        int peerSocket = accept(_listeningSocket, (struct sockaddr *)&peerAddr, &peerSize);

        // get username from inital chunk
        char buffer[1024];
        ssize_t chunk = recv(peerSocket, buffer, sizeof(buffer), 0);
        if (chunk <= 0)
        {
            throw std::runtime_error("Failed to recieve inital user chunk containg username");
            return;
        }
        buffer[chunk] = '\0';

        // send your username back to them
        send(peerSocket, _username.c_str(), _username.size(), 0);

        // send peer list
        sendPeerList(peerSocket, _username);

        // create new peer to add to peer list
        ConnectedPeer connectedPeer;
        connectedPeer.socket = peerSocket;
        connectedPeer.username = buffer;
        connectedPeer.ip = inet_ntoa(peerAddr.sin_addr);
        connectedPeer.port = ntohs(peerAddr.sin_port);

        {
            std::lock_guard<std::mutex> lock(_peerMutex);
            _connectedPeers.push_back(connectedPeer);
        }

        // create per peer thread for handling incoming messages
        std::thread peerThread(&Peer::handlePeerConnection, this, peerSocket);
        peerThread.detach();

        std::cout << "Client: " << connectedPeer.username << " Connected\n";
    }
}

void Peer::connectToPeer(std::string ip, int port, std::string username)
{
    // create client socket
    int connectingSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (connectingSocket < 0)
    {
        throw std::runtime_error("TCP Socket Failure");
    }

    // create addressc
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    if (connect(connectingSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        throw std::runtime_error("Connection Failure");
    }

    std::cout << "Connection Succesful\n";

    // send your username
    char buffer[1024];
    send(connectingSocket, username.c_str(), username.size(), 0);

    // recieve theirs back
    ssize_t chunk = recv(connectingSocket, buffer, sizeof(buffer), 0);
    if (chunk <= 0)
    {
        throw std::runtime_error("Failed to recieve inital user chunk containg username");
        return;
    }
    buffer[chunk] = '\0';

    // add them to peer list
    ConnectedPeer newPeer;
    newPeer.socket = connectingSocket;
    newPeer.username = buffer;
    newPeer.ip = ip;
    newPeer.port = port;

    {
        std::lock_guard<std::mutex> lock(_peerMutex);
        _connectedPeers.push_back(newPeer);
    }

    sendPeerList(connectingSocket, username);

    // create client thread
    std::thread receivingThread(&Peer::handlePeerConnection, this, connectingSocket);
    receivingThread.detach();
}

void Peer::handlePeerConnection(int socket)
{
    // message buffer
    char buffer[1024];

    // grab username once
    std::string username;
    {
        std::lock_guard<std::mutex> lock(Peer::_peerMutex);
        for (const auto &peer : _connectedPeers)
        {
            if (peer.socket == socket)
            {
                username = peer.username;
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
            ssize_t chunk = recv(socket, buffer, sizeof(buffer) - 1, 0);

            // users disconnected
            if (chunk <= 0)
            {
                std::cout << "User: " << username << " Disconnected\n";

                // close connection
                close(socket);

                // remove client from peerlist
                {
                    std::lock_guard<std::mutex> lock(Peer::_peerMutex);

                    // remove from peer list
                    Peer::_connectedPeers.erase(
                        std::remove_if(Peer::_connectedPeers.begin(), Peer::_connectedPeers.end(),
                                       [socket](const ConnectedPeer &p)
                                       {
                                           return p.socket == socket;
                                       }),
                        Peer::_connectedPeers.end());
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

        std::string payloadLengthStr = incomingData.substr(firstDelimiterIndex + 1, secondDelimiterIndex - firstDelimiterIndex - 1);
        int payloadLength = std::stoi(payloadLengthStr);

        // check if header already contains payload
        std::string payload = incomingData.substr(secondDelimiterIndex + 1);

        // loop again to get any missing payload data
        while (payload.length() < payloadLength)
        {
            ssize_t chunk = recv(socket, buffer, sizeof(buffer), 0);
            if (chunk <= 0)
                break;

            buffer[chunk] = '\0';
            payload.append(buffer);
        }

        if (payloadType == "MSG")
        {
            // append senders username to broadcast to others
            std::string broadcastStr = username + ": " + payload;

            // reformat messsage header to relay to the other peers
            std::string msgHeader = "RELAY|" + std::to_string(broadcastStr.length()) + "|";
            std::string fullMsg = msgHeader + broadcastStr;

            broadcastMessage(fullMsg, socket);

            std::cout << username << ": " << payload << std::endl;
        }
        else if (payloadType == "RELAY")
        {
            std::cout << payload << std::endl;
        }
        else if (payloadType == "PEERLIST")
        {

            // split by semi colon ;
            std::stringstream ss(payload);
            std::string peerEntry;

            while (std::getline(ss, peerEntry, ';'))
            {
                if (peerEntry.empty())
                    continue;

                std::stringstream peerStream(peerEntry);
                std::string username, ip, portStr, senderUsername;
                std::getline(peerStream, username, ',');
                std::getline(peerStream, ip, ',');
                std::getline(peerStream, portStr, ',');
                std::getline(peerStream, senderUsername, ',');
                int port = std::stoi(portStr);

                DiscoveredPeer discoveredPeer;
                discoveredPeer.ip = ip;
                discoveredPeer.port = port;
                discoveredPeer.username = username;
                discoveredPeer.discoveredFrom = senderUsername;

                {
                    std::lock_guard<std::mutex> lock(_peerMutex);
                    _discoveredPeers.push_back(discoveredPeer);
                }

                std::cout << "Discovered peer: " << username << " at "
                          << ip << ":" << port << " from " << senderUsername << std::endl;
            }

            // process each section and extract peer info
        }
    }
}

void Peer::broadcastMessage(std::string message, int senderSocket)
{
    {
        std::lock_guard<std::mutex> lock(_peerMutex);
        for (size_t i = 0; i < _connectedPeers.size(); i++)
        {
            if (_connectedPeers[i].socket == senderSocket)
                continue;

            send(_connectedPeers[i].socket, message.c_str(), message.size(), 0);
        }
    }
}

void Peer::userInputLoop()
{
    std::string input;
    while (std::getline(std::cin, input))
    {

        // create messsage header
        std::string msgHeader = "MSG|" + std::to_string(input.length()) + "|";
        std::string fullMsg = msgHeader + input;

        // send full msg to all peers
        {
            std::lock_guard<std::mutex> lock(_peerMutex);
            for (const auto &peer : _connectedPeers)
            {
                send(peer.socket, fullMsg.c_str(), fullMsg.size(), 0);
            }
        }
    }
}

void Peer::sendPeerList(int connectingSocket, std::string senderUsername)
{
    std::string peerListData;
    {
        std::lock_guard<std::mutex> lock(_peerMutex);
        for (const auto &peer : _connectedPeers)
        {
            peerListData += peer.username + "," +
                            peer.ip + "," +
                            std::to_string(peer.port) + "," +
                            senderUsername + ";";
        }
    }

    std::string peerHeader = "PEERLIST|" +
                             std::to_string(peerListData.length()) + "|" +
                             peerListData;
    send(connectingSocket, peerHeader.c_str(), peerHeader.size(), 0);
}

Peer::~Peer()
{
    close(_listeningSocket);
}
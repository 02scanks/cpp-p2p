#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <sstream>

struct ConnectedPeer
{
    int socket;
    std::string username;
    std::string ip;
    int port;
};

struct DiscoveredPeer
{
    std::string username;
    std::string ip;
    int port;
    std::string discoveredFrom;
};

class Peer
{
public:
    void startListening(int port);
    void connectToPeer(std::string ip, int port, std::string username);
    void handlePeerConnection(int socket);
    void broadcastMessage(std::string message, int senderSocket);
    void userInputLoop();
    void setUsername(std::string username);
    void setListeningPort(int port);
    void sendPeerList(int connectingSocket, std::string senderUsername);
    void parsePeerList(std::string payload);
    ~Peer();

private:
    std::string _username;
    int _listeningPort;
    int _listeningSocket;
    std::vector<ConnectedPeer> _connectedPeers;
    std::vector<DiscoveredPeer> _discoveredPeers;
    std::mutex _peerMutex;
};
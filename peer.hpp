#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <sstream>
#include <functional>

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
    void sendMessage(std::string message);
    void setMessageCallback(std::function<void(std::string)> callback);
    void setDiscoveredPeerCallback(std::function<void(std::string)> callback);
    std::vector<ConnectedPeer> getConnectedPeers();
    std::vector<DiscoveredPeer> getDiscoveredPeers();
    ~Peer();

private:
    std::string _username;
    int _listeningPort;
    int _listeningSocket;
    std::vector<ConnectedPeer> _connectedPeers;
    std::vector<DiscoveredPeer> _discoveredPeers;
    std::mutex _peerMutex;
    std::function<void(std::string)> _messageCallback;
    std::function<void(std::string)> _discoveredPeerCallback;
};
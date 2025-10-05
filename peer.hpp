#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <mutex>

struct PeerConnection
{
    int socket;
    std::string username;
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
    ~Peer();

private:
    std::string _username;
    int _listeningPort;
    int _listeningSocket;
    std::vector<PeerConnection> _peers;
    std::mutex _peerMutex;
};
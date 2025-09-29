#pragma once

class Client
{
public:
    ~Client();
    void receiveLoop();
    void run();

private:
    int _clientSocket;
};
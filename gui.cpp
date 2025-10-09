#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include "peer.hpp"
#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


std::vector<std::string> messages;
std::vector<std::string> peerList;

int main()
{
    using namespace ftxui;


    std::cout << "\033[34m" << R"(
/$$   /$$ /$$   /$$ /$$       /$$        /$$$$$$  /$$                   /$$    
| $$$ | $$| $$  | $$| $$      | $$       /$$__  $$| $$                  | $$    
| $$$$| $$| $$  | $$| $$      | $$      | $$  \__/| $$$$$$$   /$$$$$$  /$$$$$$  
| $$ $$ $$| $$  | $$| $$      | $$      | $$      | $$__  $$ |____  $$|_  $$_/  
| $$  $$$$| $$  | $$| $$      | $$      | $$      | $$  \ $$  /$$$$$$$  | $$    
| $$\  $$$| $$  | $$| $$      | $$      | $$    $$| $$  | $$ /$$__  $$  | $$ /$$
| $$ \  $$|  $$$$$$/| $$$$$$$$| $$$$$$$$|  $$$$$$/| $$  | $$|  $$$$$$$  |  $$$$/
|__/  \__/ \______/ |________/|________/ \______/ |__/  |__/ \_______/   \___/  

)" << std::endl;

    // CREATE PEER
    Peer peer;

    std::cout << "Enter a username for this session\n";
    std::string username;
    std::cin >> username;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    peer.setUsername(username);

    std::cout << "Enter a port to listen on\n";
    int port;
    std::cin >> port;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    peer.setListeningPort(port);

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

    // set callback
    peer.setMessageCallback([&](std::string msg){
        messages.push_back(msg);
    });

    // CREATE GUI
    std::string inputText = "";

    auto chatWindow = Renderer([&] {
          std::vector<Element> messageElements;
          for (const auto& msg : messages) {
              messageElements.push_back(text(msg));
          }
          return vbox(messageElements) | border | flex; });


    auto inputBox = Input(&inputText, "Type Here...");
    auto inputBoxWithBorder = Renderer(inputBox, [&]{
        return inputBox->Render() | border | bgcolor(Color::Default);
    });
    auto inputBoxWitHandler = CatchEvent(inputBoxWithBorder, [&](Event event) {
        if(event == Event::Return) {
            peer.sendMessage(inputText);
            std::string fromMsg = "You: " + inputText;
            messages.push_back(fromMsg);
            inputText = "";
            return true;
        }
        else {
            return false;
        }
    });

    auto chatContainer = Container::Vertical({
        chatWindow,
        inputBoxWitHandler
    });


    auto peerWindow = Renderer([&]{
        std::vector<Element> peerElements;
        peerElements.push_back(text("Connected Peers") | bold);
        for(const auto& peer : peerList) {
            peerElements.push_back(text(peer));
        }

        return vbox(peerElements) | border | size(WIDTH, EQUAL, 20);
    });

    auto mainLayout = Container::Horizontal({
        chatContainer, 
        peerWindow
    });

    auto mainLayoutRenderer = Renderer(mainLayout, [&] {
        return hbox({
            chatContainer->Render() | flex,
            peerWindow->Render()
        });
    });

    
    auto screen = ScreenInteractive::Fullscreen();

    // GUI LOOP
    screen.Loop(mainLayoutRenderer);

    return 0;
}
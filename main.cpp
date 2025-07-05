#include "video_server.h"
#include "video_client.h"
#include "network_utils.h"
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

    //The code initially asks for user to choose their server/client side of the app.
    //The decision changes the flow of the program but for overall integretiy both server and client originates from the same side
    std::cout << "Choosee Your Prefered Network Side: (S/C) ";
    char mode_in_use;
    std::cin >> mode_in_use;

    const int port_in_use = 5678;   //port chosen to be at random, sinde the program will be running on local host 127.0.0.1
    int server_side_socket = -1, client_side_socket = -1;

    if (mode_in_use == 's' || mode_in_use == 'S') {
        server_side_socket = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in address_in_use{};
        address_in_use.sin_family = AF_INET;
        address_in_use.sin_addr.s_addr = INADDR_ANY;
        address_in_use.sin_port = htons(port_in_use);

        bind(server_side_socket, (sockaddr*)&address_in_use, sizeof(address_in_use));
        listen(server_side_socket, 1);
        std::cout << "Waiting for connection..." << std::endl;
        client_side_socket = accept(server_side_socket, nullptr, nullptr);
        std::cout << "Client connected." << std::endl;

        runVideoServer(client_side_socket);

#ifdef _WIN32
        closesocket(server_side_socket);
        closesocket(client_side_socket);
#else
        close(sock);
        close(client_sock);
#endif
    } else {
        server_side_socket = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_in_use);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        std::cout << "Connecting..." << std::endl;
        if (connect(server_side_socket, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Failed to connect." << std::endl;
            return -1;
        }
        std::cout << "Connected." << std::endl;

        runVideoClient(server_side_socket);

#ifdef _WIN32
        closesocket(server_side_socket);
#else
        close(sock);
#endif
    }
    return 0;
}

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

    std::cout << "Server (s) or Client (c)? ";
    char mode;
    std::cin >> mode;

    const int PORT = 5000;
    int sock = -1, client_sock = -1;

    if (mode == 's') {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(PORT);

        bind(sock, (sockaddr*)&addr, sizeof(addr));
        listen(sock, 1);
        std::cout << "Waiting for connection..." << std::endl;
        client_sock = accept(sock, nullptr, nullptr);
        std::cout << "Client connected." << std::endl;

        runVideoServer(client_sock);

#ifdef _WIN32
        closesocket(sock);
        closesocket(client_sock);
#else
        close(sock);
        close(client_sock);
#endif
    } else {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        std::cout << "Connecting..." << std::endl;
        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Failed to connect." << std::endl;
            return -1;
        }
        std::cout << "Connected." << std::endl;

        runVideoClient(sock);

#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }
    return 0;
}

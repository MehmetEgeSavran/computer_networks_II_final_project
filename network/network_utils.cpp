#include "network_utils.h"

bool sendAll(int sock, const uint8_t* data, int length) {
    int sent = 0;
    while (sent < length) {
#ifdef _WIN32
        int send_index = send(sock, (const char*)data + sent, length - sent, 0);
#else
        int send_index = send(sock, data + sent, length - sent, 0);
#endif
        if (send_index <= 0) return false;
        sent += send_index;
    }
    return true;
}

bool recvAll(int sock, uint8_t* data, int length) {
    int recvd = 0;
    while (recvd < length) {
#ifdef _WIN32
        int recieved = recv(sock, (char*)data + recvd, length - recvd, 0);
#else
        int recieved = recv(sock, data + recvd, length - recvd, 0);
#endif
        if (recieved <= 0) return false;
        recvd += recieved;
    }
    return true;
}

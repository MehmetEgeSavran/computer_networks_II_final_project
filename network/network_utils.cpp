#include "network_utils.h"

bool sendAll(int sock, const uint8_t* data, int len) {
    int sent = 0;
    while (sent < len) {
#ifdef _WIN32
        int send_index = send(sock, (const char*)data + sent, len - sent, 0);
#else
        int send_index = send(sock, data + sent, len - sent, 0);
#endif
        if (send_index <= 0) return false;
        sent += send_index;
    }
    return true;
}

bool recvAll(int sock, uint8_t* data, int len) {
    int recvd = 0;
    while (recvd < len) {
#ifdef _WIN32
        int r = recv(sock, (char*)data + recvd, len - recvd, 0);
#else
        int r = recv(sock, data + recvd, len - recvd, 0);
#endif
        if (r <= 0) return false;
        recvd += r;
    }
    return true;
}

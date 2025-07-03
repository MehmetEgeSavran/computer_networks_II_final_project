#pragma once
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

bool sendAll(int sock, const uint8_t* data, int len);
bool recvAll(int sock, uint8_t* data, int len);

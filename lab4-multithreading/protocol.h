#ifndef LAB4_PROTOCOL_H
#define LAB4_PROTOCOL_H

#include <winsock2.h>
#include <cstdint>
#include <cstdio>

constexpr unsigned short PORT = 666;

struct Message {
    std::uint32_t number;  // В сети хранится в сетевом порядке байтов.
    char text[128];        // До 127 байт текста и завершающий ноль.
};
static_assert(sizeof(Message) == 132, "Unexpected Message layout");

inline SOCKET openTcpSocket() {
    WSADATA data;
    int error = WSAStartup(MAKEWORD(2, 2), &data);
    if (error != 0) {
        std::fprintf(stderr, "WSAStartup failed: %d\n", error);
        return INVALID_SOCKET;
    }
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
        WSACleanup();
    }
    return sock;
}

// TCP может принять на отправку только часть структуры за один вызов send.
inline bool sendMessage(SOCKET sock, const Message& message) {
    const char* bytes = reinterpret_cast<const char*>(&message);
    const int size = sizeof(message);
    int sent = 0;
    while (sent < size) {
        int count = send(sock, bytes + sent, size - sent, 0);
        if (count <= 0) {
            return false;
        }
        sent += count;
    }
    return true;
}

// Возвращает число прочитанных байтов или SOCKET_ERROR при ошибке.
inline int receiveMessage(SOCKET sock, Message& message) {
    char* bytes = reinterpret_cast<char*>(&message);
    const int size = sizeof(message);
    int received = 0;
    while (received < size) {
        int count = recv(sock, bytes + received, size - received, 0);
        if (count == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }
        if (count == 0) {
            break;  // Клиент или сервер закрыл соединение.
        }
        received += count;
    }
    return received;
}

#endif

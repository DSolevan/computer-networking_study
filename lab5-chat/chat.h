#pragma once

#include <winsock2.h>
#include <string>

const int PORT = 12345;
const int MAX_INPUT = 1000;  // Длина строки пользователя в байтах.
const int MAX_PACKET = 2048; // Оставляем место для ника и подписи сервера.

enum Packet { Nick = 1, Public, Private, Notice };

// TCP может передать за один вызов только часть данных.
inline bool sendAll(SOCKET s, const char* data, int size) {
    while (size > 0) {
        int sent = send(s, data, size, 0);
        if (sent <= 0) return false;
        data += sent;
        size -= sent;
    }
    return true;
}

inline bool receiveAll(SOCKET s, char* data, int size) {
    while (size > 0) {
        int received = recv(s, data, size, 0);
        if (received <= 0) return false;
        data += received;
        size -= received;
    }
    return true;
}

// Пакет состоит из типа, длины текста в байтах и самого текста.
inline bool sendPacket(SOCKET s, Packet type, const std::string& text) {
    if (text.size() > MAX_PACKET) return false;
    unsigned short header[2] = {htons(type), htons(static_cast<unsigned short>(text.size()))};
    return sendAll(s, reinterpret_cast<const char*>(header), sizeof(header))
        && sendAll(s, text.data(), static_cast<int>(text.size()));
}

inline bool receivePacket(SOCKET s, Packet& type, std::string& text) {
    unsigned short header[2];
    if (!receiveAll(s, reinterpret_cast<char*>(header), sizeof(header))) return false;

    int code = ntohs(header[0]);
    int size = ntohs(header[1]);
    if (code < Nick || code > Notice || size > MAX_PACKET) return false;
    type = static_cast<Packet>(code);
    text.resize(size);
    return size == 0 || receiveAll(s, &text[0], size);
}

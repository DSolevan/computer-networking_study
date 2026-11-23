#include "protocol.h"
#include <ws2tcpip.h>
#include <cstring>
#include <iostream>
#include <string>

int runClient(SOCKET sock, const char* ip) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    if (inet_pton(AF_INET, ip, &address.sin_addr) != 1) {
        std::cerr << "Invalid IPv4 address: " << ip << '\n';
        return 1;
    }
    if (connect(sock, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        std::cerr << "connect failed: " << error << '\n';
        return 1;
    }
    std::cout << "Connected to " << ip << ':' << PORT << ". Type quit to exit.\n";

    std::uint32_t number = 1;
    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line)) {
        if (line == "quit") {
            break;
        }
        Message request{};
        if (line.size() >= sizeof(request.text)) {
            std::cout << "Use at most " << sizeof(request.text) - 1 << " bytes.\n";
            continue;
        }
        request.number = htonl(number);
        std::memcpy(request.text, line.c_str(), line.size() + 1);
        if (!sendMessage(sock, request)) {
            std::cerr << "Cannot send request.\n";
            return 1;
        }
        Message reply{};
        if (receiveMessage(sock, reply) != sizeof(Message)) {
            std::cerr << "Connection lost before a complete reply arrived.\n";
            return 1;
        }
        if (std::memcmp(&request, &reply, sizeof(Message)) != 0) {
            std::cerr << "Echo differs from the request.\n";
            return 1;
        }
        std::cout << "Echo #" << ntohl(reply.number) << ": " << reply.text << '\n';
        ++number;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: client [server-IPv4]\n";
        return 1;
    }
    const char* ip = argc == 2 ? argv[1] : "127.0.0.1";
    SOCKET sock = openTcpSocket();
    if (sock == INVALID_SOCKET) {
        return 1;
    }
    int result = runClient(sock, ip);
    closesocket(sock);
    WSACleanup();
    return result;
}

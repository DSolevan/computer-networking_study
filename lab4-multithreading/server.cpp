#include "protocol.h"
#include <windows.h>
#include <new>

DWORD WINAPI serveClient(void* argument) {
    SOCKET client = *static_cast<SOCKET*>(argument);
    delete static_cast<SOCKET*>(argument);
    DWORD id = GetCurrentThreadId();
    std::printf("[thread %lu] connected\n", id);

    Message message{};
    while (true) {
        int received = receiveMessage(client, message);
        if (received == 0) {
            break;
        }
        if (received != sizeof(Message)) {
            std::fprintf(stderr, "[thread %lu] incomplete message or receive error\n", id);
            break;
        }
        // Отправляем те же байты тому клиенту, от которого их получили.
        if (!sendMessage(client, message)) {
            std::fprintf(stderr, "[thread %lu] send failed\n", id);
            break;
        }
        std::printf("[thread %lu] echo #%lu: %.127s\n", id,
                    static_cast<unsigned long>(ntohl(message.number)), message.text);
    }

    closesocket(client);
    std::printf("[thread %lu] disconnected\n", id);
    return 0;
}

int runServer(SOCKET listener) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        std::fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
        return 1;
    }
    if (listen(listener, SOMAXCONN) == SOCKET_ERROR) {
        std::fprintf(stderr, "listen failed: %d\n", WSAGetLastError());
        return 1;
    }
    std::printf("TCP echo server listening on port %u\n", static_cast<unsigned>(PORT));

    while (true) {
        SOCKET client = accept(listener, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            int error = WSAGetLastError();
            if (error == WSAECONNRESET) {
                continue;  // Клиент успел отключиться до accept.
            }
            std::fprintf(stderr, "accept failed: %d\n", error);
            return 1;
        }
        // У каждого потока своя копия сокета. Следующий accept её не перезапишет.
        SOCKET* argument = new (std::nothrow) SOCKET(client);
        HANDLE thread = nullptr;
        if (argument != nullptr) {
            thread = CreateThread(nullptr, 0, serveClient, argument, 0, nullptr);
        }
        if (thread == nullptr) {
            std::fprintf(stderr, "Cannot start a client thread\n");
            delete argument;
            closesocket(client);
            continue;
        }
        CloseHandle(thread);  // Поток продолжает работу. Главный поток вернётся к accept.
    }
}

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    SOCKET listener = openTcpSocket();
    if (listener == INVALID_SOCKET) {
        return 1;
    }
    int result = runServer(listener);
    closesocket(listener);
    WSACleanup();
    return result;
}

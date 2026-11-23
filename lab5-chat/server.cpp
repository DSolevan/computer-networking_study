#include "chat.h"
#include <windows.h>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

using namespace std;

map<string, SOCKET> clients; // По нику находим сокет участника.
mutex chatMutex;

void deliver(SOCKET s, Packet type, const string& text) {
    if (!sendPacket(s, type, text)) shutdown(s, SD_BOTH);
}

// Вызывается под chatMutex, чтобы потоки не перемешивали пакеты.
void broadcast(Packet type, const string& text, SOCKET except = INVALID_SOCKET) {
    for (const auto& client : clients) {
        if (client.second != except) deliver(client.second, type, text);
    }
}

bool joinChat(SOCKET s, const string& nick) {
    if (nick.empty() || nick.size() > 32 || nick.find_first_of(" \t\r\n") != string::npos) {
        sendPacket(s, Notice, "Ник должен занимать от 1 до 32 байт, без пробелов.");
        return false;
    }

    lock_guard<mutex> lock(chatMutex);
    if (clients.count(nick)) {
        sendPacket(s, Notice, "Этот ник уже занят.");
        return false;
    }
    if (!sendPacket(s, Nick, "Вы вошли в чат.")) return false;

    clients[nick] = s;
    string notice = nick + " входит в чат.";
    broadcast(Notice, notice, s);
    cout << notice << endl;
    return true;
}

void handleClient(SOCKET s) {
    Packet type;
    string nick, text;
    if (!receivePacket(s, type, nick) || type != Nick || !joinChat(s, nick)) {
        closesocket(s);
        return;
    }

    while (receivePacket(s, type, text)) {
        if (text.size() > MAX_INPUT) break;
        lock_guard<mutex> lock(chatMutex);
        if (type == Public) {
            broadcast(Public, nick + ": " + text);
        } else if (type == Private) {
            // В личном сообщении до первого пробела записан ник адресата.
            size_t space = text.find(' ');
            if (space == string::npos || space == 0 || space + 1 == text.size()) {
                deliver(s, Notice, "Формат: /w ник текст");
                continue;
            }
            string recipient = text.substr(0, space);
            auto it = clients.find(recipient);
            if (it == clients.end()) {
                deliver(s, Notice, "Участника " + recipient + " нет в чате.");
            } else {
                deliver(it->second, Private, "[Лично] " + nick + ": " + text.substr(space + 1));
            }
        } else {
            break;
        }
    }

    lock_guard<mutex> lock(chatMutex);
    clients.erase(nick);
    closesocket(s);
    string notice = nick + " выходит из чата.";
    broadcast(Notice, notice);
    cout << notice << endl;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cerr << "Не удалось запустить WinSock.\n";
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // Принимаем подключения и по локальной сети.
    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET ||
        bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR ||
        listen(listener, SOMAXCONN) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        cerr << "Не удалось открыть порт " << PORT << ". Ошибка WinSock " << error << endl;
        if (listener != INVALID_SOCKET) closesocket(listener);
        WSACleanup();
        return 1;
    }

    cout << "Сервер ждёт клиентов. Порт " << PORT << endl;
    while (true) {
        SOCKET s = accept(listener, nullptr, nullptr);
        if (s == INVALID_SOCKET) break;

        // Ограничиваем ожидание записи, если клиент перестал читать данные.
        DWORD timeout = 3000;
        setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        thread(handleClient, s).detach();
    }
    closesocket(listener);
    WSACleanup();
    return 0;
}

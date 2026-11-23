#include "chat.h"
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <thread>

using namespace std;

void receiveMessages(SOCKET s) {
    Packet type;
    string text;
    while (receivePacket(s, type, text)) {
        cout << text << endl;
    }
    cout << "Соединение закрыто." << endl;
}

int runChat(SOCKET s) {
    string ip, nick;
    cout << "IPv4 сервера (Enter для 127.0.0.1): ";
    if (!getline(cin, ip)) return 1;
    if (ip.empty()) ip = "127.0.0.1";

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        cerr << "Неверный IPv4-адрес.\n";
        return 1;
    }
    if (connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        cerr << "Не удалось подключиться. Ошибка WinSock " << error << endl;
        return 1;
    }

    cout << "Ваш ник: ";
    if (!getline(cin, nick)) return 1;
    Packet reply;
    string text;
    if (!sendPacket(s, Nick, nick) || !receivePacket(s, reply, text)) {
        cerr << "Сервер не подтвердил вход.\n";
        return 1;
    }
    cout << text << endl;
    if (reply != Nick) return 1;

    cout << "Обычная строка — в общий чат, /w ник текст — лично, /quit — выход." << endl;
    thread receiver(receiveMessages, s);
    string line;
    while (getline(cin, line) && line != "/quit") {
        if (line.empty()) continue;
        if (line.size() > MAX_INPUT) {
            cout << "Строка слишком длинная (максимум " << MAX_INPUT << " байт)." << endl;
            continue;
        }
        if (line == "/w") {
            cout << "Формат: /w ник текст" << endl;
            continue;
        }
        Packet type = Public;
        if (line.compare(0, 3, "/w ") == 0) {
            type = Private;
            line.erase(0, 3);
        }
        if (!sendPacket(s, type, line)) break;
    }
    shutdown(s, SD_SEND); // Сервер увидит конец ввода и закроет соединение.
    receiver.join();
    return 0;
}

int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cerr << "Не удалось запустить WinSock.\n";
        return 1;
    }
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        cerr << "Не удалось создать сокет.\n";
        WSACleanup();
        return 1;
    }
    int result = runChat(s);
    closesocket(s);
    WSACleanup();
    return result;
}

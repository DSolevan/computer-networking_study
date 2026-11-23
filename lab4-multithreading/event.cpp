#include <windows.h>
#include <cstdio>

HANDLE canWrite;
HANDLE canRead;
int values[5]{};
constexpr int ROUNDS = 4;

DWORD WINAPI writer(void*) {
    for (int n = 1; n <= ROUNDS; ++n) {
        WaitForSingleObject(canWrite, INFINITE);
        for (int& value : values) {
            value = n;
        }
        SetEvent(canRead);  // Массив целиком заполнен.
    }
    return 0;
}

int main() {
    // Оба события с автосбросом. Сначала разрешена только запись.
    canWrite = CreateEvent(nullptr, FALSE, TRUE, nullptr);
    canRead = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!canRead || !canWrite) {
        std::fprintf(stderr, "CreateEvent failed\n");
        if (canRead) CloseHandle(canRead);
        if (canWrite) CloseHandle(canWrite);
        return 1;
    }
    HANDLE thread = CreateThread(nullptr, 0, writer, nullptr, 0, nullptr);
    if (!thread) {
        std::fprintf(stderr, "CreateThread failed\n");
        CloseHandle(canWrite);
        CloseHandle(canRead);
        return 1;
    }

    for (int n = 1; n <= ROUNDS; ++n) {
        WaitForSingleObject(canRead, INFINITE);
        std::printf("Read:");
        for (int value : values) {
            std::printf(" %d", value);
        }
        std::puts("");
        SetEvent(canWrite);  // Предыдущая строка прочитана, можно записывать следующую.
    }
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    CloseHandle(canRead);
    CloseHandle(canWrite);
    return 0;
}

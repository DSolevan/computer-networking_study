#include <windows.h>
#include <cstdio>

HANDLE counterMutex;
int counter = 0;
constexpr int INCREMENTS = 5000;

// Оба потока захватывают один и тот же мьютекс перед изменением счётчика.
void incrementCounter() {
    for (int i = 0; i < INCREMENTS; ++i) {
        WaitForSingleObject(counterMutex, INFINITE);
        ++counter;
        ReleaseMutex(counterMutex);
    }
}

DWORD WINAPI worker(void*) {
    incrementCounter();
    return 0;
}

int main() {
    counterMutex = CreateMutex(nullptr, FALSE, nullptr);
    if (!counterMutex) {
        std::fprintf(stderr, "CreateMutex failed\n");
        return 1;
    }
    HANDLE thread = CreateThread(nullptr, 0, worker, nullptr, 0, nullptr);
    if (!thread) {
        std::fprintf(stderr, "CreateThread failed\n");
        CloseHandle(counterMutex);
        return 1;
    }
    incrementCounter();
    WaitForSingleObject(thread, INFINITE);
    std::printf("2 threads x %d increments\nExpected: %d\nActual:   %d\n",
                INCREMENTS, 2 * INCREMENTS, counter);
    CloseHandle(thread);
    CloseHandle(counterMutex);
    return counter == 2 * INCREMENTS ? 0 : 1;
}

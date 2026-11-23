#include <windows.h>
#include <cstdio>

constexpr int CAPACITY = 2;
constexpr int ITEMS = 6;
int buffer[CAPACITY]{};
HANDLE freeSlots;  // Сколько ячеек свободно.
HANDLE usedSlots;  // Сколько значений можно прочитать.

DWORD WINAPI producer(void*) {
    for (int i = 0; i < ITEMS; ++i) {
        WaitForSingleObject(freeSlots, INFINITE);
        int slot = i % CAPACITY;
        buffer[slot] = i + 1;
        std::printf("Write %d -> slot %d\n", buffer[slot], slot);
        ReleaseSemaphore(usedSlots, 1, nullptr);
    }
    return 0;
}

int main() {
    freeSlots = CreateSemaphore(nullptr, CAPACITY, CAPACITY, nullptr);
    usedSlots = CreateSemaphore(nullptr, 0, CAPACITY, nullptr);
    if (!freeSlots || !usedSlots) {
        std::fprintf(stderr, "CreateSemaphore failed\n");
        if (freeSlots) CloseHandle(freeSlots);
        if (usedSlots) CloseHandle(usedSlots);
        return 1;
    }
    HANDLE thread = CreateThread(nullptr, 0, producer, nullptr, 0, nullptr);
    if (!thread) {
        std::fprintf(stderr, "CreateThread failed\n");
        CloseHandle(freeSlots);
        CloseHandle(usedSlots);
        return 1;
    }

    for (int i = 0; i < ITEMS; ++i) {
        WaitForSingleObject(usedSlots, INFINITE);
        int slot = i % CAPACITY;
        std::printf("Read  %d <- slot %d\n", buffer[slot], slot);
        ReleaseSemaphore(freeSlots, 1, nullptr);
    }
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    CloseHandle(usedSlots);
    CloseHandle(freeSlots);
    return 0;
}

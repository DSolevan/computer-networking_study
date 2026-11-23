#include <windows.h>
#include <cstdio>

CRITICAL_SECTION section;
int values[5]{};

// Основной и рабочий потоки по три раза увеличивают весь массив.
void incrementArray(const char* name) {
    for (int step = 0; step < 3; ++step) {
        EnterCriticalSection(&section);
        for (int& value : values) {
            ++value;
        }
        std::printf("%-6s", name);
        for (int value : values) {
            std::printf(" %d", value);
        }
        std::puts("");
        LeaveCriticalSection(&section);
        Sleep(10);  // Пауза лишь делает чередование заметнее.
    }
}

DWORD WINAPI worker(void*) {
    incrementArray("worker");
    return 0;
}

int main() {
    InitializeCriticalSection(&section);
    HANDLE thread = CreateThread(nullptr, 0, worker, nullptr, 0, nullptr);
    if (!thread) {
        std::fprintf(stderr, "CreateThread failed\n");
        DeleteCriticalSection(&section);
        return 1;
    }
    incrementArray("main");
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    DeleteCriticalSection(&section);
    return 0;
}

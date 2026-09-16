// EXPECT_COMPILE_ARGS: -lws2_32 -ladvapi32
#include <winsock2.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    WSADATA sockets;
    SYSTEM_INFO system;
    HKEY key;
    char *memory, *end;
    FILE *file;
    double value = strtod("12.5 tail", &end);
    if (value != 12.5 || strcmp(end, " tail")) return 1;
    GetSystemInfo(&system);
    memory = VirtualAlloc(0, system.dwPageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!memory) return 2;
    strcpy(memory, "portable Windows runtime");
    if (strcmp(memory, "portable Windows runtime")) return 3;
    if (!VirtualFree(memory, 0, MEM_RELEASE)) return 4;
    if (WSAStartup(MAKEWORD(2, 2), &sockets)) return 5;
    if (WSACleanup()) return 6;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software", 0, KEY_READ, &key)) return 7;
    if (RegCloseKey(key)) return 8;
    file = tmpfile();
    if (!file) return 9;
    if (fputs("round trip", file) < 0 || fseek(file, 0, SEEK_SET)) return 10;
    {
        char text[32];
        if (!fgets(text, sizeof(text), file) || strcmp(text, "round trip")) return 11;
    }
    return fclose(file) != 0;
}

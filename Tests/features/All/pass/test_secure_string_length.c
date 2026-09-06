// EXPECT_EXIT: 0
#include <string.h>
#include <windows.h>

int main(void)
{
    SYSTEM_INFO info;
    char *pages;
    char *text;
    DWORD old_protection;
    size_t (*length)(const char *, size_t) = strnlen_s;
    int result = 0;
    if (length(NULL, 0) != 0 || length(NULL, 42) != 0) return 1;
    if (length("abcdef", 0) != 0 || length("abcdef", 3) != 3 ||
        length("abcdef", 20) != 6 || length("ab\0cd", 5) != 2) return 2;
    GetSystemInfo(&info);
    pages = (char *)VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!pages) return 3;
    if (!VirtualProtect(pages + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &old_protection)) {
        VirtualFree(pages, 0, MEM_RELEASE);
        return 4;
    }
    text = pages + info.dwPageSize - 3;
    text[0] = 'a'; text[1] = 'b'; text[2] = 'c';
    if (length(text, 3) != 3) result = 5;
    text[1] = '\0';
    if (length(text, 3) != 1) result = 6;
    VirtualFree(pages, 0, MEM_RELEASE);
    return result;
}

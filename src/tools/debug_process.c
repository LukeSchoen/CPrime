/* Five-second Windows startup diagnostics.
   Usage: debug_process.exe program [cwd [client-capture.bmp]].
   Supplying a capture path shows the child window; otherwise it stays hidden. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
WINBASEAPI DWORD WINAPI GetFinalPathNameByHandleA(HANDLE, LPSTR, DWORD, DWORD);

static DWORD child_pid;
static const char *capture_path;
static void capture_window(HWND window)
{
    RECT rect;
    HDC screen, memory;
    HBITMAP bitmap;
    BITMAPINFO info = {0};
    BITMAPFILEHEADER header = {0};
    void *pixels;
    FILE *file;
    if (!GetClientRect(window, &rect) || rect.right <= 0 || rect.bottom <= 0) return;
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = rect.right;
    info.bmiHeader.biHeight = -rect.bottom;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    screen = GetDC(window);
    memory = CreateCompatibleDC(screen);
    bitmap = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &pixels, NULL, 0);
    if (bitmap) {
        HGDIOBJ old = SelectObject(memory, bitmap);
        BitBlt(memory, 0, 0, rect.right, rect.bottom, screen, 0, 0, SRCCOPY);
        header.bfType = 0x4d42;
        header.bfOffBits = sizeof(header) + sizeof(BITMAPINFOHEADER);
        header.bfSize = header.bfOffBits + rect.right * rect.bottom * 4;
        file = fopen(capture_path, "wb");
        if (file) {
            fwrite(&header, sizeof(header), 1, file);
            fwrite(&info.bmiHeader, sizeof(BITMAPINFOHEADER), 1, file);
            fwrite(pixels, rect.right * rect.bottom * 4, 1, file);
            fclose(file);
        }
        SelectObject(memory, old);
        DeleteObject(bitmap);
    }
    DeleteDC(memory);
    ReleaseDC(window, screen);
}
static BOOL CALLBACK report_window(HWND window, LPARAM unused)
{
    DWORD pid;
    char title[256];
    (void)unused;
    GetWindowThreadProcessId(window, &pid);
    if (pid == child_pid) {
        GetWindowTextA(window, title, sizeof(title));
        printf("window %p visible=%d title=%s\n", window, IsWindowVisible(window), title);
        if (capture_path && IsWindowVisible(window) && title[0]) capture_window(window);
    }
    return TRUE;
}

int main(int argc, char **argv)
{
    STARTUPINFOA startup = {0};
    PROCESS_INFORMATION process = {0};
    DEBUG_EVENT event;
    DWORD started;
    DWORD exit_code = 124;
    int finished = 0;
    if (argc < 2) return 2;
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    capture_path = argc > 3 ? argv[3] : NULL;
    startup.wShowWindow = capture_path ? SW_SHOWNORMAL : SW_HIDE;
    if (!CreateProcessA(argv[1], NULL, NULL, NULL, FALSE,
                        DEBUG_ONLY_THIS_PROCESS, NULL, argc > 2 ? argv[2] : NULL,
                        &startup, &process)) {
        printf("CreateProcess error %lu\n", GetLastError()); return 2;
    }
    child_pid = process.dwProcessId;
    started = GetTickCount();
    while (GetTickCount() - started < 5000) {
        DWORD continuation = DBG_CONTINUE;
        if (!WaitForDebugEvent(&event, 50)) continue;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            printf("image=%p entry=%p\n", event.u.CreateProcessInfo.lpBaseOfImage,
                   event.u.CreateProcessInfo.lpStartAddress);
            if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            char path[1024] = {0};
            if (event.u.LoadDll.hFile) {
                GetFinalPathNameByHandleA(event.u.LoadDll.hFile, path, sizeof(path), 0);
                CloseHandle(event.u.LoadDll.hFile);
            }
            printf("module=%p %s\n", event.u.LoadDll.lpBaseOfDll, path);
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            printf("exception=%08lx address=%p first=%lu\n",
                   event.u.Exception.ExceptionRecord.ExceptionCode,
                   event.u.Exception.ExceptionRecord.ExceptionAddress,
                   event.u.Exception.dwFirstChance);
            if (event.u.Exception.ExceptionRecord.ExceptionCode != EXCEPTION_BREAKPOINT)
                continuation = DBG_EXCEPTION_NOT_HANDLED;
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            printf("exit=%lu\n", event.u.ExitProcess.dwExitCode);
            exit_code = event.u.ExitProcess.dwExitCode;
            finished = 1;
        }
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, continuation);
        if (finished) break;
    }
    if (!finished) {
        CONTEXT context = {0};
        unsigned long long stack[32];
        SIZE_T bytes = 0;
        unsigned i;
        SuspendThread(process.hThread);
        context.ContextFlags = CONTEXT_FULL;
        if (GetThreadContext(process.hThread, &context)) {
            printf("timeout rip=%llx rsp=%llx rbp=%llx\n", context.Rip, context.Rsp, context.Rbp);
            ReadProcessMemory(process.hProcess, (void *)context.Rsp, stack, sizeof(stack), &bytes);
            for (i = 0; i < bytes / sizeof(stack[0]); ++i)
                printf("stack+%02x=%llx\n", i * 8, stack[i]);
        }
        EnumWindows(report_window, 0);
        TerminateProcess(process.hProcess, 124);
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return (int)exit_code;
}

/* Bounded Windows crash tracing for CPC reproducers. Pass one command-line
   string containing the compiler and its arguments. Build this tool with CPC. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct MapSymbol { unsigned long long address; char name[256]; } MapSymbol;
static MapSymbol *symbols;
static int symbol_count;
static int symbol_capacity;
static unsigned long long symbol_limit;

static void print_stack(HANDLE process, DWORD thread_id) {
    HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, thread_id);
    CONTEXT context;
    unsigned long long pc, fp;
    unsigned i;
    if (!thread) return;
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context)) { CloseHandle(thread); return; }
    pc = context.Rip; fp = context.Rbp;
    printf("pc=0x%llx sp=0x%llx fp=0x%llx\n", pc,
           (unsigned long long)context.Rsp, fp);
    /* CPC emits frame-pointer chains. This tool is deliberately scoped to
       those compiler builds, not arbitrary native optimized executables. */
    for (i = 0; i < 24 && pc; ++i) {
        unsigned long long pair[2];
        SIZE_T read;
        int j, best = -1;
        for (j = 0; j < symbol_count; ++j)
            if (symbols[j].address <= pc && (best < 0 || symbols[j].address > symbols[best].address)) best = j;
        if (best >= 0 && pc <= symbol_limit) printf("%02u %s + 0x%llx\n", i, symbols[best].name, pc - symbols[best].address);
        else printf("%02u 0x%llx\n", i, pc);
        if (!fp || !ReadProcessMemory(process, (void *)fp, pair, sizeof(pair), &read)
            || read != sizeof(pair) || pair[0] <= fp) break;
        fp = pair[0]; pc = pair[1];
    }
    /* Stack overflow can stop in an OS stack-probe stub without a usable
       frame chain. Report bounded raw stack candidates separately. */
    {
        unsigned long long page = context.Rsp & ~4095ULL;
        unsigned long long maximum = 0;
        int j, printed = 0;
        for (j = 0; j < symbol_count; ++j)
            if (symbols[j].address > maximum && symbols[j].address < 0x100000000ULL)
                maximum = symbols[j].address;
        for (i = 0; i < 32 && printed < 64; ++i, page += 4096) {
            unsigned long long words[512];
            SIZE_T bytes;
            unsigned k;
            if (!ReadProcessMemory(process, (void *)page, words, sizeof(words), &bytes)) continue;
            for (k = 0; k < bytes / 8 && printed < 64; ++k) {
                int best = -1;
                if (words[k] < 0x401000 || words[k] >= maximum) continue;
                for (j = 0; j < symbol_count; ++j)
                    if (symbols[j].address <= words[k] && (best < 0 || symbols[j].address > symbols[best].address)) best = j;
                if (best >= 0) {
                    printf("stack candidate %s + 0x%llx\n", symbols[best].name, words[k] - symbols[best].address);
                    ++printed;
                }
            }
        }
    }
    CloseHandle(thread);
}

int main(int argc, char **argv) {
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    DEBUG_EVENT event;
    DWORD start;
    int result = 1, finished = 0;
    if (argc != 3) { fprintf(stderr, "usage: trace_compile \"compiler arguments\" compiler.map\n"); return 2; }
    {
        FILE *map = fopen(argv[2], "r");
        MapSymbol symbol;
        if (!map) return 2;
        while (fscanf(map, "%llx %255s", &symbol.address, symbol.name) == 2) {
            if (symbol_count == symbol_capacity) {
                int capacity = symbol_capacity ? symbol_capacity * 2 : 256;
                MapSymbol *grown = realloc(symbols, capacity * sizeof(*symbols));
                if (!grown) { fclose(map); free(symbols); return 2; }
                symbols = grown; symbol_capacity = capacity;
            }
            symbols[symbol_count++] = symbol;
            if (symbol.address > symbol_limit && symbol.address < 0x100000000ULL)
                symbol_limit = symbol.address;
        }
        fclose(map);
    }
    memset(&startup, 0, sizeof(startup));
    memset(&process, 0, sizeof(process));
    startup.cb = sizeof(startup);
    if (!CreateProcessA(NULL, argv[1], NULL, NULL, FALSE,
                       DEBUG_ONLY_THIS_PROCESS | CREATE_NO_WINDOW, NULL, NULL, &startup, &process)) {
        fprintf(stderr, "CreateProcess failed: %lu\n", GetLastError()); return 2;
    }
    start = GetTickCount();
    while ((DWORD)(GetTickCount() - start) < 5000) {
        DWORD action = DBG_CONTINUE;
        if (!WaitForDebugEvent(&event, 50)) continue;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT && event.u.CreateProcessInfo.hFile)
            CloseHandle(event.u.CreateProcessInfo.hFile);
        if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT && event.u.LoadDll.hFile)
            CloseHandle(event.u.LoadDll.hFile);
        if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            DWORD code = event.u.Exception.ExceptionRecord.ExceptionCode;
            if (!event.u.Exception.dwFirstChance || code == EXCEPTION_STACK_OVERFLOW
                || code == EXCEPTION_ACCESS_VIOLATION) {
                printf("Compiler exception 0x%08lx\n", code);
                print_stack(process.hProcess, event.dwThreadId);
                TerminateProcess(process.hProcess, 1);
                ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE);
                break;
            }
            if (code != EXCEPTION_BREAKPOINT) action = DBG_EXCEPTION_NOT_HANDLED;
        }
        if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            result = event.u.ExitProcess.dwExitCode == 0 ? 0 : 1;
            finished = 1;
        }
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, action);
        if (finished) break;
    }
    if (!finished) {
        if ((DWORD)(GetTickCount() - start) >= 5000
            && SuspendThread(process.hThread) != (DWORD)-1) {
            print_stack(process.hProcess, process.dwThreadId);
            ResumeThread(process.hThread);
        }
        TerminateProcess(process.hProcess, 1);
    }
    WaitForSingleObject(process.hProcess, 1000);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    free(symbols);
    return result;
}


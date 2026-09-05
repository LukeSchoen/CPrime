/* Small Windows/CRT compatibility entry points that are not exported by the
   legacy msvcrt import library used by the PE runtime.  Keep these in the
   owned runtime so generated programs never need project-specific shims. */

#include <stddef.h>
#include <stdio.h>
#include <stdatomic.h>
#include <io.h>

size_t strnlen_s(const char *text, size_t maximum)
{
    size_t length = 0;
    if (!text)
        return 0;
    while (length < maximum && text[length])
        ++length;
    return length;
}

long long _ftelli64(FILE *stream)
{
    return _telli64(_fileno(stream));
}

#include <windows.h>
#include <stdlib.h>
#include <cprime_thread.h>

typedef struct CpcWindowsMutex {
    int recursive;
    union { SRWLOCK lock; CRITICAL_SECTION recursive_lock; } native;
} CpcWindowsMutex;

void *__cpc_windows_mutex_create(int recursive)
{
    CpcWindowsMutex *mutex = malloc(sizeof(*mutex));
    if (!mutex) return NULL;
    mutex->recursive = recursive;
    if (recursive) InitializeCriticalSection(&mutex->native.recursive_lock);
    else InitializeSRWLock(&mutex->native.lock);
    return mutex;
}

void __cpc_windows_mutex_destroy(void *handle)
{
    CpcWindowsMutex *mutex = handle;
    if (mutex->recursive) DeleteCriticalSection(&mutex->native.recursive_lock);
    free(mutex);
}

void __cpc_windows_mutex_lock(void *handle)
{
    CpcWindowsMutex *mutex = handle;
    if (mutex->recursive) EnterCriticalSection(&mutex->native.recursive_lock);
    else AcquireSRWLockExclusive(&mutex->native.lock);
}

int __cpc_windows_mutex_try_lock(void *handle)
{
    CpcWindowsMutex *mutex = handle;
    return mutex->recursive
        ? TryEnterCriticalSection(&mutex->native.recursive_lock) != 0
        : TryAcquireSRWLockExclusive(&mutex->native.lock) != 0;
}

void __cpc_windows_mutex_unlock(void *handle)
{
    CpcWindowsMutex *mutex = handle;
    if (mutex->recursive) LeaveCriticalSection(&mutex->native.recursive_lock);
    else ReleaseSRWLockExclusive(&mutex->native.lock);
}

void *__cpc_windows_condition_create(void)
{
    CONDITION_VARIABLE *condition = malloc(sizeof(*condition));
    if (condition) InitializeConditionVariable(condition);
    return condition;
}

void __cpc_windows_condition_destroy(void *condition) { free(condition); }

void __cpc_windows_condition_wait(void *condition, void *handle)
{
    CpcWindowsMutex *mutex = handle;
    SleepConditionVariableSRW(condition, &mutex->native.lock, INFINITE, 0);
}

void __cpc_windows_condition_notify(void *condition, int all)
{
    if (all) WakeAllConditionVariable(condition);
    else WakeConditionVariable(condition);
}

typedef struct CpcWindowsThreadStart {
    void (*entry)(void *);
    void *argument;
} CpcWindowsThreadStart;

static DWORD WINAPI cpc_windows_thread_entry(void *argument)
{
    CpcWindowsThreadStart start = *(CpcWindowsThreadStart *)argument;
    free(argument);
    start.entry(start.argument);
    return 0;
}

void *__cpc_windows_thread_create(void (*entry)(void *), void *argument)
{
    CpcWindowsThreadStart *start = malloc(sizeof(*start));
    HANDLE thread;
    if (!start) return NULL;
    start->entry = entry;
    start->argument = argument;
    thread = CreateThread(NULL, 0, cpc_windows_thread_entry, start, 0, NULL);
    if (!thread) free(start);
    return thread;
}

void __cpc_windows_thread_join(void *thread)
{
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

void __cpc_windows_thread_detach(void *thread) { CloseHandle(thread); }
unsigned int __cpc_windows_thread_id(void) { return GetCurrentThreadId(); }
unsigned int __cpc_windows_thread_handle_id(void *thread) { return GetThreadId(thread); }

unsigned int __cpc_windows_hardware_concurrency(void)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors ? info.dwNumberOfProcessors : 1;
}

void __cpc_windows_sleep_for_milliseconds(long long milliseconds)
{
    Sleep(milliseconds > 0 ? (DWORD)milliseconds : 0);
}

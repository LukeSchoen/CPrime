/* Small Windows/CRT compatibility entry points that are not exported by the
   legacy msvcrt import library used by the PE runtime.  Keep these in the
   owned runtime so generated programs never need project-specific shims. */

#include <stddef.h>
#include <stdio.h>
#include <stdatomic.h>
#include <io.h>

#ifndef __CPRIME_UCRT__
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
#endif

#include <windows.h>
#include <stdlib.h>
#include <cprime_thread.h>

#ifdef __CPRIME_UCRT__
/* Native Windows objects reference this CRT marker when they use floating
   point.  x64 uses SSE directly and needs no x87 emulation initializer. */
int _fltused = 0x9875;
#endif

/* Compiler-owned static initialization guards. A completed guard needs only
   an atomic load; waiting and exception retries share a process-local lock. */
static SRWLOCK cpc_static_lock = SRWLOCK_INIT;
static CONDITION_VARIABLE cpc_static_condition = CONDITION_VARIABLE_INIT;

int __cpc_static_acquire(void *storage)
{
    volatile LONG *guard = storage;
    int initialize;
    if (InterlockedCompareExchange(guard, 2, 2) == 2) return 0;
    AcquireSRWLockExclusive(&cpc_static_lock);
    while (*guard == 1)
        SleepConditionVariableSRW(&cpc_static_condition, &cpc_static_lock,
                                 INFINITE, 0);
    initialize = *guard == 0;
    if (initialize) *guard = 1;
    ReleaseSRWLockExclusive(&cpc_static_lock);
    return initialize;
}

static void cpc_static_finish(volatile LONG *guard, LONG state)
{
    AcquireSRWLockExclusive(&cpc_static_lock);
    InterlockedExchange(guard, state);
    WakeAllConditionVariable(&cpc_static_condition);
    ReleaseSRWLockExclusive(&cpc_static_lock);
}

void __cpc_static_release(void *guard) { cpc_static_finish(guard, 2); }
void __cpc_static_abort(void *guard) { cpc_static_finish(guard, 0); }

typedef struct CpcStaticDestructor {
    struct CpcStaticDestructor *previous;
    void *object;
    void (*destroy)(void *);
    unsigned long long count, size;
} CpcStaticDestructor;
static CpcStaticDestructor *cpc_static_destructors;
#if !defined(__CPRIME_NATIVE_CRT__) || !defined(__CPRIME_UCRT__)
static int cpc_static_exit_registered;
#endif

void __cpc_static_destroy_elements(void *object, void (*destroy)(void *),
                                    unsigned long long count,
                                    unsigned long long size)
{
    while (count) destroy((char *)object + --count * size);
}

void __cpc_run_static_destructors(void)
{
    for (;;) {
        CpcStaticDestructor *entry;
        AcquireSRWLockExclusive(&cpc_static_lock);
        entry = cpc_static_destructors;
        if (entry) cpc_static_destructors = entry->previous;
        ReleaseSRWLockExclusive(&cpc_static_lock);
        if (!entry) return;
        __cpc_static_destroy_elements(entry->object, entry->destroy,
                                       entry->count, entry->size);
        free(entry);
    }
}

#if defined(__CPRIME_NATIVE_CRT__) && defined(__CPRIME_UCRT__)
static void cpc_run_one_static_destructor(void)
{
    CpcStaticDestructor *entry;
    AcquireSRWLockExclusive(&cpc_static_lock);
    entry = cpc_static_destructors;
    if (entry) cpc_static_destructors = entry->previous;
    ReleaseSRWLockExclusive(&cpc_static_lock);
    if (!entry) return;
    __cpc_static_destroy_elements(entry->object, entry->destroy,
                                  entry->count, entry->size);
    free(entry);
}
#endif

void __cpc_register_static_destructor(void *object, void (*destroy)(void *),
                                      unsigned long long count,
                                      unsigned long long size)
{
    CpcStaticDestructor *entry = malloc(sizeof(*entry));
    if (!entry) abort();
    entry->object = object;
    entry->destroy = destroy;
    entry->count = count;
    entry->size = size;
    AcquireSRWLockExclusive(&cpc_static_lock);
#if !defined(__CPRIME_NATIVE_CRT__) || !defined(__CPRIME_UCRT__)
    if (!cpc_static_exit_registered) {
        HMODULE module;
        /* DLL detach drains its own registry. Do not leave a callback into
           an unloaded DLL in the process CRT's atexit list. */
        if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
                                  | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)__cpc_run_static_destructors, &module)
            || module == GetModuleHandleA(NULL))
            if (atexit(__cpc_run_static_destructors)) abort();
        cpc_static_exit_registered = 1;
    }
#endif
    entry->previous = cpc_static_destructors;
    cpc_static_destructors = entry;
#if defined(__CPRIME_NATIVE_CRT__) && defined(__CPRIME_UCRT__)
    /* Each completed object occupies its own position among ordinary exit
       callbacks. The module CRT also owns these callbacks for DLLs and -run. */
    if (atexit(cpc_run_one_static_destructor)) abort();
#endif
    ReleaseSRWLockExclusive(&cpc_static_lock);
}

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

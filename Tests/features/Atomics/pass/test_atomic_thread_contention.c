// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <stdatomic.h>
#include <windows.h>

enum { worker_count = 4, iterations = 10000 };
static unsigned char count1, cas1;
static unsigned short count2, cas2;
static unsigned int count4, cas4;
static unsigned long long count8, cas8;
static atomic_flag guard = ATOMIC_FLAG_INIT;
static int protected_count;

#define INCREMENT_CAS(TYPE, OBJECT) \
    do { \
        TYPE expected, desired; \
        __atomic_load(&(OBJECT), &expected, __ATOMIC_RELAXED); \
        do { desired = expected + 1; } \
        while (!__atomic_compare_exchange(&(OBJECT), &expected, &desired, 1, \
                   __ATOMIC_RELAXED, __ATOMIC_RELAXED)); \
    } while (0)

static DWORD WINAPI worker(void *unused) {
    int i;
    for (i = 0; i < iterations; ++i) {
        __atomic_fetch_add(&count1, 1, __ATOMIC_RELAXED);
        __atomic_fetch_add(&count2, 1, __ATOMIC_RELAXED);
        __atomic_fetch_add(&count4, 1, __ATOMIC_RELAXED);
        __atomic_fetch_add(&count8, 1, __ATOMIC_RELAXED);
        INCREMENT_CAS(unsigned char, cas1);
        INCREMENT_CAS(unsigned short, cas2);
        INCREMENT_CAS(unsigned int, cas4);
        INCREMENT_CAS(unsigned long long, cas8);
        while (atomic_flag_test_and_set_explicit(&guard, memory_order_acquire)) {}
        ++protected_count;
        atomic_flag_clear_explicit(&guard, memory_order_release);
    }
    return 0;
}

static unsigned int published;
static int payload, publication_failed;

static DWORD WINAPI consumer(void *unused) {
    int i;
    for (i = 1; i <= iterations; ++i) {
        while (!__atomic_load_n(&published, __ATOMIC_ACQUIRE)) {}
        if (payload != i) publication_failed = 1;
        __atomic_store_n(&published, 0, __ATOMIC_RELEASE);
    }
    return 0;
}

int main(void) {
    HANDLE workers[worker_count], reader;
    int i, expected = worker_count * iterations;
    for (i = 0; i < worker_count; ++i) {
        workers[i] = CreateThread(0, 0, worker, 0, 0, 0);
        if (!workers[i]) return 1;
    }
    for (i = 0; i < worker_count; ++i) {
        if (WaitForSingleObject(workers[i], 10000) != WAIT_OBJECT_0) return 2;
        CloseHandle(workers[i]);
    }
    if (count1 != (unsigned char)expected || cas1 != count1) return 3;
    if (count2 != (unsigned short)expected || cas2 != count2) return 4;
    if (count4 != expected || cas4 != count4 || count8 != expected || cas8 != count8) return 5;
    if (protected_count != expected) return 6;
    reader = CreateThread(0, 0, consumer, 0, 0, 0);
    if (!reader) return 7;
    for (i = 1; i <= iterations; ++i) {
        while (__atomic_load_n(&published, __ATOMIC_ACQUIRE)) {}
        payload = i;
        __atomic_store_n(&published, 1, __ATOMIC_RELEASE);
    }
    if (WaitForSingleObject(reader, 10000) != WAIT_OBJECT_0) return 8;
    CloseHandle(reader);
    return publication_failed ? 9 : 0;
}

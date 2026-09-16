extern "C" {
extern __declspec(thread) int native_tls_value;
extern __declspec(thread) int native_tls_zero;
extern __declspec(thread) int native_tls_aligned;
__declspec(thread) int native_tls_dynamic;
__declspec(thread) int native_tls_destroyed;
int native_tls_destructor_count;
int puts(const char *);
int atexit(void (*)());
extern int native_init_order;
extern int native_process_order;
void late_callback(void *, unsigned long reason, void *) {
    if (reason == 1 || reason == 2) {
        native_tls_value += 200;
        native_tls_zero = native_tls_zero * 10 + 2;
        if (reason == 1) native_process_order = native_process_order * 10 + 2;
    }
}
int late_c_init() { native_init_order = native_init_order * 10 + 2; return 0; }
int native_tls_check(int expected, int replacement) {
    if (native_tls_value != expected || native_tls_zero != 12) return 1;
    if (native_tls_dynamic != 43 || native_tls_aligned != 41 ||
        (reinterpret_cast<unsigned long long>(&native_tls_aligned) & 63)) return 2;
    native_tls_value = replacement;
    return 0;
}
void dynamic_tls_init(void *, unsigned long reason, void *) {
    if (reason == 2) native_tls_dynamic = 43;
}
void dynamic_tls_dtor(void *, unsigned long reason, void *) {
    if ((reason == 0 || reason == 3) && !native_tls_destroyed) {
        native_tls_destroyed = 1;
        ++native_tls_destructor_count;
        puts("tls-dtor");
    }
}
extern void (*const __dyn_tls_init_callback)(void *, unsigned long, void *) = dynamic_tls_init;
extern void (*const __dyn_tls_dtor_callback)(void *, unsigned long, void *) = dynamic_tls_dtor;
void native_atexit() { puts(native_tls_destructor_count == 2 ? "atexit" : "bad-tls-order"); }
void native_preterminate() { puts("preterminate"); }
void native_terminate() { puts("terminate"); }
#pragma section(".CRT$XLC", read)
#pragma section(".CRT$XIU", read)
#pragma section(".CRT$XLD", read)
#pragma section(".CRT$XLE", read)
#pragma section(".CRT$XPU", read)
#pragma section(".CRT$XTU", read)
__declspec(allocate(".CRT$XLC")) void (*late_tls)(void *, unsigned long, void *) = late_callback;
__declspec(allocate(".CRT$XIU")) int (*late_c)() = late_c_init;
__declspec(allocate(".CRT$XLD")) void (*init_tls)(void *, unsigned long, void *) = dynamic_tls_init;
__declspec(allocate(".CRT$XLE")) void (*dtor_tls)(void *, unsigned long, void *) = dynamic_tls_dtor;
__declspec(allocate(".CRT$XPU")) void (*preterminate)() = native_preterminate;
__declspec(allocate(".CRT$XTU")) void (*terminate)() = native_terminate;
}
struct Global {
    Global() { native_init_order = native_init_order * 10 + 3; atexit(native_atexit); }
} global;

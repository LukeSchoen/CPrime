extern "C" {
__declspec(thread) int native_tls_value = 17;
__declspec(thread) int native_tls_zero;
__declspec(align(64)) __declspec(thread) int native_tls_aligned = 41;
int native_init_order;
int native_process_order;
void early_callback(void *, unsigned long reason, void *) {
    if (reason == 1 || reason == 2) {
        native_tls_value += 100;
        native_tls_zero = 1;
        if (reason == 1) native_process_order = 1;
    }
}
int early_c_init() { native_init_order = native_init_order * 10 + 1; return 0; }
#pragma section(".CRT$XLB", read)
#pragma section(".CRT$XIAB", read)
__declspec(allocate(".CRT$XLB")) void (*early_tls)(void *, unsigned long, void *) = early_callback;
__declspec(allocate(".CRT$XIAB")) int (*early_c)() = early_c_init;
}

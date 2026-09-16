// Standard PE TLS directory and sentinel sections, as supplied by a native CRT.
typedef void (*Callback)(void *, unsigned long, void *);
extern "C" {
unsigned long _tls_index;
#pragma section(".tls", read, write)
__declspec(allocate(".tls")) char _tls_start;
#pragma section(".tls$ZZZ", read, write)
__declspec(allocate(".tls$ZZZ")) char _tls_end;
#pragma section(".CRT$XLA", read)
#pragma section(".CRT$XLZ", read)
__declspec(allocate(".CRT$XLA")) Callback __xl_a = 0;
__declspec(allocate(".CRT$XLZ")) Callback __xl_z = 0;
struct TlsDirectory {
    const void *start;
    const void *end;
    const void *index;
    const void *callbacks;
    unsigned long zero_fill;
    unsigned long characteristics;
};
extern const TlsDirectory _tls_used = {
    &_tls_start, &_tls_end, &_tls_index, &__xl_a + 1, 0, 0
};
}

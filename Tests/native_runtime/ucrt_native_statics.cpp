#pragma comment(lib, "libcmt")
static int construction_count;
struct NativeValue {
    int value;
    NativeValue() : value(42) { ++construction_count; }
};
extern "C" int native_static_value()
{
    static NativeValue value;
    return value.value;
}
extern "C" int native_construction_count() { return construction_count; }
extern "C" int native_tls_increment()
{
    static __declspec(thread) int value = 7;
    return ++value;
}

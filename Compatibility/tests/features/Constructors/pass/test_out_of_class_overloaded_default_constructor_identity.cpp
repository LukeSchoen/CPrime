// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
struct Writer {
    int value;
    Writer(long long capacity = 8LL * 1024LL * 1024LL);
    Writer(double value);
    Writer(const char* name, long long capacity = 19, bool append = false);
    Writer(Writer&& other);
    virtual ~Writer();
};
// Definitions intentionally use a different order than declarations.
Writer::Writer(Writer&& other) : value(other.value) { other.value = 0; }
Writer::Writer(double input) : value((int)input + 1) {}
Writer::Writer(const char* name, long long capacity, bool append)
    : value((int)capacity + (append ? 1 : 0)) {}
Writer::Writer(long long capacity) : value((int)capacity) {}
Writer::~Writer() {}
int main() {
    Writer automatic;
    Writer named("file");
    Writer number(3.5);
    Writer sized(12LL);
    Writer moved(static_cast<Writer&&>(sized));
    if (automatic.value != 8388608 || named.value != 19) return 1;
    if (number.value != 4 || moved.value != 12 || sized.value != 0) return 2;
    return 0;
}

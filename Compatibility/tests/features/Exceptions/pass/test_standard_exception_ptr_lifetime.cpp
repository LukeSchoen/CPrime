#include <exception>
static int destroyed;
struct Error {
    int value;
    Error(int value) : value(value) {}
    ~Error() { ++destroyed; }
};
int main() {
    std::exception_ptr retained;
    if (retained || std::current_exception()) return 1;
    try { throw Error(167); }
    catch (...) { retained = std::current_exception(); }
    if (!retained || destroyed) return 2;
    {
        std::exception_ptr copy = retained;
        if (copy != retained) return 3;
        try { std::rethrow_exception(copy); }
        catch (const Error& error) { if (error.value != 167 || destroyed) return 4; }
    }
    retained = std::exception_ptr();
    return destroyed != 1;
}

#include <exception>
static int destroyed;
struct Error {
    ~Error() { ++destroyed; }
};
static int from_return(const Error& error) {
    try { throw error; }
    catch (const Error&) { return 11; }
    return 0;
}
int main() {
    Error error;
    if (from_return(error) != 11 || destroyed != 1) return 1;
    for (int index = 0; index != 3; ++index) {
        try { throw error; }
        catch (const Error&) { if (index < 2) continue; break; }
    }
    if (destroyed != 4) return 2;
    try { throw error; }
    catch (const Error&) { goto finished; }
    return 3;
finished:
    return destroyed != 5 || std::uncaught_exceptions() != 0;
}

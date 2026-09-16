#include <exception>
#include <stdlib.h>
static void terminate_check() {
    _Exit(std::uncaught_exceptions() == 2 ? 0 : 2);
}
struct Guard {
    ~Guard() noexcept(false) { throw 137; }
};
int main() {
    std::set_terminate(terminate_check);
    try { Guard guard; throw 139; }
    catch (...) { return 1; }
    return 3;
}

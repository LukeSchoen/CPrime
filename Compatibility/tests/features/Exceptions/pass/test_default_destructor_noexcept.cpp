#include <exception>
#include <stdlib.h>
static void terminate_check() {
    _Exit(std::uncaught_exceptions() == 1 ? 0 : 2);
}
struct DefaultNoexcept {
    ~DefaultNoexcept() { throw 113; }
};
int main() {
    std::set_terminate(terminate_check);
    try { DefaultNoexcept object; }
    catch (...) { return 1; }
    return 3;
}

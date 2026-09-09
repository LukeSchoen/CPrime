#include <exception>
#include <cstdlib>
void terminate_handler() {
    if (!std::current_exception()) std::exit(1);
    try { throw; }
    catch (int value) { std::exit(value == 17 ? 0 : 2); }
    catch (...) { std::exit(3); }
}
void fail() noexcept { throw 17; }
int main() {
    std::set_terminate(terminate_handler);
    try { throw 9; }
    catch (int) { fail(); }
    return 4;
}

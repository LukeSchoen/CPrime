#include <exception>
static int caught;
struct Guard {
    ~Guard() {
        if (std::uncaught_exceptions() != 1) return;
        try { throw 127; }
        catch (int value) {
            if (value == 127 && std::uncaught_exceptions() == 1) caught = 1;
        }
    }
};
int main() {
    try { Guard guard; throw 131; }
    catch (int value) {
        if (value != 131 || std::uncaught_exceptions()) return 1;
        return caught != 1;
    }
    return 2;
}

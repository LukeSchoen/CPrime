#include <typeinfo>
#include <string.h>
struct Base { virtual ~Base() {} };
struct Derived : Base {};
int main() {
    Base object;
    try { (void)dynamic_cast<Derived&>(object); return 1; }
    catch (const std::bad_cast& error) {
        if (strcmp(error.what(), "std::bad_cast")) return 2;
    }
    try { (void)dynamic_cast<Derived&>(object); return 3; }
    catch (const std::exception&) { return 0; }
    return 4;
}

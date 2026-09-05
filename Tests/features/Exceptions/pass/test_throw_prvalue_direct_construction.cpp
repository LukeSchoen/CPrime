static int constructed, destroyed;
struct Error {
    const Error *self;
    int value;
    Error(int value) : self(this), value(value) { ++constructed; }
    Error(const Error&) = delete;
    ~Error() { ++destroyed; }
};
struct Trivial { int value; };
static Trivial make_trivial() { Trivial result; result.value = 151; return result; }
namespace codes { enum { value = 157 }; }
int main() {
    try { throw Error(149); }
    catch (const Error& error) {
        if (error.self != &error || error.value != 149 || constructed != 1 || destroyed) return 1;
    }
    if (destroyed != 1) return 2;
    try { throw make_trivial(); }
    catch (const Trivial& error) { if (error.value != 151) return 3; }
    try { int flag = 1; int unused = flag ? throw codes::value : 0; }
    catch (int code) { if (code != 157) return 4; }
    return 0;
}

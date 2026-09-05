static int constructed, destroyed;
struct Error {
    const Error *self;
    Error() : self(this) { ++constructed; }
    Error(const Error&) = delete;
    ~Error() { ++destroyed; }
};
static Error make_error() { return Error(); }
int main() {
    {
        Error value = make_error();
        if (value.self != &value || constructed != 1 || destroyed) return 1;
    }
    if (destroyed != 1) return 2;
    try { throw make_error(); }
    catch (const Error& error) {
        if (error.self != &error || constructed != 2 || destroyed != 1) return 3;
    }
    return destroyed != 2;
}

int arguments, objects, snapshot;
bool thrown;
struct Argument {
    Argument() { ++arguments; }
    ~Argument() noexcept(false) {
        --arguments;
        if (!thrown) { thrown = true; throw 17; }
    }
};
int consume(const Argument&, const Argument&) { return 1; }
struct Object {
    Object(int) { ++objects; }
    ~Object() { snapshot = arguments; --objects; }
};
int main() {
    try { Object object(consume(Argument(), Argument())); }
    catch (int value) { return value != 17 || arguments || objects || snapshot != 1; }
    return 2;
}

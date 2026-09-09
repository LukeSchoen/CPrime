int alive, failed;
struct Argument {
    Argument() { ++alive; }
    ~Argument() { --alive; }
};
struct Element {
    Element(int expected, const Argument& = Argument()) {
        if (alive != expected) failed = 1;
    }
    Element(const Argument& = Argument()) {
        if (alive != 2) failed = 2;
    }
};
int main() {
    Element explicit_elements[] = {1, 2, 3};
    if (alive || failed) return 1;
    Element partial[3] = {1};
    return alive || failed;
}

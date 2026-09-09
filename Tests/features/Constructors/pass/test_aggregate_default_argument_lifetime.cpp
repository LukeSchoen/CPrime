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
struct Members { Element first, second; };
struct Array { Element elements[3]; };
int main() {
    Members members = {1, 2};
    if (alive) return 10 + alive;
    if (failed) return 20 + failed;
    Array array = {1};
    return alive || failed;
}

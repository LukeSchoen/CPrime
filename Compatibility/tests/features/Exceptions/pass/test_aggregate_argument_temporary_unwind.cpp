int arguments, elements, fail_element, throw_argument;
struct Argument {
    Argument() { ++arguments; }
    ~Argument() noexcept(false) {
        --arguments;
        if (throw_argument && arguments == 0) throw 9;
    }
};
struct Element {
    Element(int index, const Argument& = Argument()) {
        if (index == fail_element) throw index;
        ++elements;
    }
    ~Element() { --elements; }
};
struct Aggregate { Element first, second; };
int main() {
    for (fail_element = 1; fail_element <= 2; ++fail_element) {
        try { Aggregate object = {1, 2}; return 1; }
        catch (int value) { if (value != fail_element) return 2; }
        if (arguments || elements) return 3;
    }
    fail_element = 0;
    throw_argument = 1;
    try { Aggregate object = {1, 2}; return 4; }
    catch (int value) { if (value != 9) return 5; }
    return arguments || elements;
}

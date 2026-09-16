int live, throw_at;
struct Argument {
    ~Argument() noexcept(false) { if (live == throw_at) throw live; }
};
struct Element {
    Element(const Argument& = Argument()) { ++live; }
    ~Element() { --live; }
};
int main() {
    for (throw_at = 0; throw_at <= 3; ++throw_at) {
        try {
            Element values[3];
            if (live != 3 || throw_at) return 1;
        } catch (int value) {
            if (value != throw_at) return 2;
        }
        if (live) return 3;
    }
}

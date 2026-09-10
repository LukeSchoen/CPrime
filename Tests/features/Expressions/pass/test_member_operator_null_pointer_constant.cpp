// EXPECT_EXIT: 0
// The integer constant zero is a null pointer constant, so it selects a member
// operator taking a pointer even when the class also converts from integers.
struct Probe {
    Probe() : value(0) {}
    Probe(const long n) : value((int)n) {}
    int operator ==(const char* const text) const {
        return text == 0 && value == 3;
    }
    int value;
};

int main() {
    Probe probe(3);
    if (!(probe == 0)) return 1;
    if (probe == "text") return 2;
    return 0;
}

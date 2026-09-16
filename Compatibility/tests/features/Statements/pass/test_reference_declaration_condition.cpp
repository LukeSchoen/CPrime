struct Condition {
    static int live;
    int value;
    Condition(int v) : value(v) { ++live; }
    ~Condition() { --live; }
    operator bool() const { return value != 0; }
};
int Condition::live;
int main() {
    Condition original(0);
    if (Condition& reference = original) return 1;
    if (const Condition& temporary = 3) {
        if (temporary.value != 3 || Condition::live != 2) return 2;
    } else return 3;
    return Condition::live != 1 ? 4 : 0;
}

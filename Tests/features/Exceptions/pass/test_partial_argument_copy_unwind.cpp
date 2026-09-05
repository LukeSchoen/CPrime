static int copies, destroyed_copies, entered;
struct Value {
    bool copied;
    Value() : copied(false) {}
    Value(const Value&) : copied(true) { if (++copies == 2) throw 179; }
    ~Value() { if (copied) ++destroyed_copies; }
};
static void consume(Value first, Value second) { ++entered; }
int main() {
    Value first, second;
    try { consume(first, second); }
    catch (int code) {
        return code != 179 || copies != 2 || destroyed_copies != 1 || entered;
    }
    return 2;
}

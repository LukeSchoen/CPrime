static int alive, destroyed;
struct Condition {
    int value;
    Condition(int value) : value(value) { ++alive; }
    ~Condition() { --alive; ++destroyed; }
    operator int() const { return value; }
};
int return_from_switch() {
    switch (Condition value = 1) { case 1: return alive; }
    return 0;
}
int main() {
    switch (Condition value = 1) { case 1: if (alive != 1) return 1; }
    if (alive || destroyed != 1) return 2;
    switch (Condition value = 2) { case 2: if (alive != 1) return 3; break; }
    if (alive || destroyed != 2) return 4;
    switch (Condition value = 3) { case 0: return 5; }
    if (alive || destroyed != 3) return 6;
    for (int i = 0; i < 2; ++i) {
        switch (Condition value = i) { default: continue; }
    }
    if (alive || destroyed != 5) return 7;
    if (return_from_switch() != 1 || alive || destroyed != 6) return 8;
    try { switch (Condition value = 1) { default: throw 9; } }
    catch (int value) { if (value != 9) return 10; }
    return alive || destroyed != 7;
}

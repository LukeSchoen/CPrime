struct Condition {
    int value;
    Condition(int v) : value(v) {}
    Condition(const Condition& other) : value(other.value) {}
    ~Condition() {}
    operator bool() { return value != 0; }
};
template<class T> int check() {
    int visits = 0, steps = 0;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; T condition = j < ({ if (++steps > 32) return 19; if (i == 1 && j == 1) continue; 4; }); ++j)
            ++visits;
    if (visits != 9) return 1;
    visits = steps = 0;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; T condition = j < ({ if (++steps > 32) return 29; if (i == 1 && j == 1) break; 4; }); ++j)
            ++visits;
    if (visits != 5) return 2;
    visits = steps = 0;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; T condition = j < 4; ({ if (++steps > 32) return 39; if (i == 1 && j == 1) continue; 1; }), ++j)
            ++visits;
    if (visits != 10) return 3;
    visits = steps = 0;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; T condition = j < 4; ({ if (++steps > 32) return 49; if (i == 1 && j == 1) break; 1; }), ++j)
            ++visits;
    return visits != 6 ? 4 : 0;
}

int main() { int result = check<int>(); return result ? result : check<Condition>(); }

// EXPECT_EXIT: 0
int live;
struct Guard { Guard() { ++live; } ~Guard() { --live; } };
template<class T> int recover(T value) try {
    Guard guard;
    throw value;
} catch (int caught) {
    return live ? -1 : caught;
} catch (...) {
    return live ? -2 : 11;
}
template<class T> int typed(T value) try { throw value; }
catch (T caught) { return (int)value + (int)caught; }
int after() { return 3; }
int main() {
    return recover(7) != 7 || recover(2.5) != 11 || after() != 3
        || typed(4) != 8 || typed(1.5) != 2;
}

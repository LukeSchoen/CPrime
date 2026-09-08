// EXPECT_EXIT: 0
int choose(bool) { return 1; }
int choose(int) { return 2; }
int main() {
    int a = 2, b = 3;
    return choose(a < b) != 1 || choose(a == b) != 1
        || choose(&a != &b) != 1 || choose(!a) != 1
        || sizeof(a < b) != sizeof(bool)
        || sizeof(1.0 >= 2.0) != sizeof(bool);
}

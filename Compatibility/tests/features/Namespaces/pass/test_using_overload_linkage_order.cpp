namespace original {
    int choose(int);
    int choose(double);
}
namespace original {
    int choose(int value) { return value + 10; }
    int choose(double value) { return (int)value + 20; }
    int choose(const char *value) { return *value + 30; }
}
// Register aliases through different scopes and through an earlier alias.
namespace first { using original::choose; }
namespace second { using original::choose; }
namespace third { using first::choose; }
namespace last { using original::choose; }
int main() {
    if (first::choose(1) != 11 || second::choose(2.0) != 22) return 1;
    if (third::choose(3) != 13 || last::choose(4.0) != 24) return 2;
    return last::choose("A") != 95;
}

#include <utility>
static int moves, assignments;
class MoveOnly {
public:
    int value;
    explicit MoveOnly(int input) : value(input) {}
    MoveOnly(const MoveOnly &) = delete;
    MoveOnly &operator=(const MoveOnly &) = delete;
    MoveOnly(MoveOnly &&other) : value(other.value) { other.value = -1; ++moves; }
    MoveOnly &operator=(MoveOnly &&other) {
        value = other.value;
        other.value = -1;
        ++assignments;
        return *this;
    }
};
int main() {
    MoveOnly first(17), second(29);
    std::swap(first, second);
    if (first.value != 29 || second.value != 17) return 1;
    if (moves != 1 || assignments != 2) return 2;
    int a = 3, b = 7;
    std::swap(a, b);
    return a == 7 && b == 3 ? 0 : 3;
}

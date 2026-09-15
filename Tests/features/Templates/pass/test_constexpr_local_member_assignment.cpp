struct Pair { int first; int second; };
constexpr int update() { Pair value{1, 2}; value.first = 7; value.second += 3; return value.first + value.second; }
static_assert(update() == 12);
int main() { return update() != 12; }

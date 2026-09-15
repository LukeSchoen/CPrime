struct Pair { int x; int y; };
constexpr int copies() { Pair first{3, 4}; Pair second{first}; return second.x + second.y; }
static_assert(copies() == 7);
int main() { Pair first{5, 6}; Pair second{first}; return copies() != 7 || second.x != 5 || second.y != 6; }
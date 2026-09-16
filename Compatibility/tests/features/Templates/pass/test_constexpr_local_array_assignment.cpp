constexpr int update() { int values[2] = {1, 2}; values[0] = 7; values[1] += 3; return values[0] + values[1]; }
static_assert(update() == 12);
int main() { return update() != 12; }

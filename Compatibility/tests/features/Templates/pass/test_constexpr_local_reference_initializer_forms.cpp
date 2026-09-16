constexpr int value = 9;
constexpr int direct() { const int &alias(value); return alias; }
constexpr int list() { const int &alias{value}; return alias; }
constexpr int copy_list() { const int &alias = {value,}; return alias; }
constexpr int parenthesized() { const int &alias = (0, value); return alias; }
static_assert(direct() == 9);
static_assert(list() == 9);
static_assert(copy_list() == 9);
static_assert(parenthesized() == 9);
int main() {
    return direct() != 9 || list() != 9 || copy_list() != 9 || parenthesized() != 9;
}

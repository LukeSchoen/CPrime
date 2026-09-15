constexpr int direct() { int value(9); return value; }
constexpr int list() { int value{7}; return value; }
constexpr int copy_list() { int value = {4,}; return value; }
constexpr int zero() { int value{}; return value; }
constexpr int narrow_constant() { unsigned char value{255}; return value; }
static_assert(direct() == 9);
static_assert(list() == 7);
static_assert(copy_list() == 4);
static_assert(zero() == 0);
static_assert(narrow_constant() == 255);
int main() {
    return direct() != 9 || list() != 7 || copy_list() != 4
        || zero() != 0 || narrow_constant() != 255;
}

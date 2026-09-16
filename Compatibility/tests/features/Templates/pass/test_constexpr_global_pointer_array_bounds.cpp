constexpr int values[2][2] = {{1, 2}, {3, 4}};
constexpr const int *row = values[1];
constexpr const int *alias = row;
constexpr const int *past = alias + 2;
constexpr const int *empty = nullptr;
static_assert(alias[1] == 4);
static_assert(past - row == 2);
static_assert(empty == nullptr);
static_assert(sizeof(row[2]) == sizeof(int));
int main() { return alias[1] != 4 || past - row != 2 || empty != nullptr; }

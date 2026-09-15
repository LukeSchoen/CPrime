struct Tag { explicit Tag() = default; };
constexpr Tag tag{};
struct Data { int number; explicit Data() = default; };
constexpr Data data{};
static_assert(data.number == 0);
int main() { return sizeof(tag) != 1; }

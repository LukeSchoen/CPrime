constexpr const int *empty() { return nullptr; }
constexpr const int *pointer = empty();
static_assert(pointer == nullptr);
static_assert(empty() == nullptr);
int main() { return empty() != nullptr; }

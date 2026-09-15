const int value = 9;
constexpr const int *pointer = &value;
static_assert(*pointer == 9);
const short small = -3;
constexpr const short *small_pointer = &small;
static_assert(*small_pointer == -3);
int main() { return *pointer != 9; }

int side_effect();
constexpr int inner() { return 3; }
constexpr int valid() {
  return (sizeof(side_effect()), false && side_effect(), true || side_effect(), inner(), 7);
}
static_assert(valid() == 7);
int main() { return valid() != 7; }

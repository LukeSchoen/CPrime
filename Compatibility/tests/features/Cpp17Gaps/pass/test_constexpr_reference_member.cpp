// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: constexpr_reference_member. Reading and writing through a
// reference member, with the referenced object staying inside the evaluation.
struct Ref {
  int &target;
};

constexpr int read_through_reference() {
  int value = 3;
  Ref ref{value};
  int first = ref.target;
  ref.target = 9;
  return first * 100 + value;
}

static_assert(read_through_reference() == 309, "reference member aliases its object");

int main() { return 0; }

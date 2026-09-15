// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: alignas. alignas is accepted on a local object already; the
// class-specifier form must set the record alignment too.
struct alignas(16) Aligned { int value; };

int main() {
  if (alignof(Aligned) != 16) return 1;
  alignas(16) char buffer[16];
  (void)buffer;
  Aligned object{3};
  return object.value - 3;
}

struct Floats { float values[4]; };
struct Bits { unsigned values[4]; };
static_assert(__builtin_bit_cast(unsigned, 1.0f) == 0x3f800000u, "constant bits");
static_assert(__builtin_bit_cast(float, 0x3f800000u) == 1.0f, "constant float");
int calls;
float next() { ++calls; return 2.0f; }
int main() {
  Floats input = {{ 1, 2, 3, 4 }};
  Bits output = __builtin_bit_cast(Bits, input);
  input.values[1] = 7;
  int array[4] = { 0x3f800000, 0x40000000, 0x40400000, 0x40800000 };
  return output.values[1] != 0x40000000u
      || __builtin_bit_cast(Floats, array).values[2] != 3
      || __builtin_bit_cast(unsigned, next()) != 0x40000000u || calls != 1;
}

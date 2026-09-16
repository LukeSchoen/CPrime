// A using-declaration introduces an inherited in-class integral constant so
// unqualified uses inside the derived class keep the folded value, including
// in alignas, aligned attributes and vector_size.

template <int N>
struct Constant
{
  static const int Value = N;
};

template <class T, int N>
struct Sizes : Constant<N>
{
  using Base = Constant<N>;
  using Base::Value;

  alignas(Value) char aligned[1];
  char __attribute__((aligned(Value))) attributed[1];
  T element __attribute__((vector_size(Value)));

  static_assert(Value == N, "using-declared constant is not visible");

  int value() const { return Value; }
  int element_size() const { return sizeof(element); }
};

struct Plain
{
  static const int Size = 8;
  static const int Count = 3;
};

struct Imported : Plain
{
  using Plain::Size;

  int storage __attribute__((vector_size(Size)));

  static int size() { return Size; }
};

int main()
{
  Sizes<char, 16> bytes = {};
  Sizes<int, 16> words = {};
  if (bytes.value() != 16 || words.value() != 16) { return 1; }
  if (bytes.element_size() != 16 || words.element_size() != 16) { return 2; }
  if (bytes.element[15] != 0 || words.element[3] != 0) { return 3; }
  if (Sizes<char, 16>::Value != 16) { return 4; }

  Imported imported = {};
  if (imported.size() != 8) { return 5; }
  if (sizeof(imported.storage) != 8 || imported.storage[1] != 0) { return 6; }
  return 0;
}

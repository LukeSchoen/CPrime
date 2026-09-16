// EXPECT_EXIT: 0
int evaluated, released;
int defaults;
int default_argument() { ++defaults; return 11; }
void* operator new(__SIZE_TYPE__ size, int, int = default_argument()) { return ::operator new(size); }
void operator delete(void* pointer, int first, int second) {
  if (first == 1 && second == 11) ++released;
  ::operator delete(pointer);
}
struct Value { Value() { throw 7; } };
int argument() { return ++evaluated; }
int main() {
  try { new (argument()) Value; }
  catch (int value) { if (value != 7) return 1; }
  if (evaluated != 1 || defaults != 1) return 2;
  return released != 1 ? 3 : 0;
}

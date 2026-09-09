// EXPECT_EXIT: 0
int converted, released, constructed;
struct Tag { int value; } tag = {13};
struct Convert { operator Tag&() { ++converted; return tag; } };
void* operator new(__SIZE_TYPE__ size, Tag& argument, double value) {
  if (&argument != &tag || value != 2.5) throw 1;
  return ::operator new(size);
}
void operator delete(void* pointer, Tag& argument, double value) {
  if (&argument == &tag && value == 2.5) ++released;
  ::operator delete(pointer);
}
struct Value {
  Value(int fail) { ++constructed; if (fail) throw 7; }
};
void* operator new(__SIZE_TYPE__, int*) noexcept { return 0; }
void operator delete(void*, int*) noexcept { released += 100; }
int main() {
  try { new (Convert(), 2.5) Value(1); }
  catch (int code) { if (code != 7) return 1; }
  if (converted != 1 || released != 1 || constructed != 1) return 2;
  Value* success = new (Convert(), 2.5) Value(0);
  if (converted != 2 || released != 1 || constructed != 2) return 3;
  delete success;
  Value* missing = new ((int*)0) Value(1);
  return missing || released != 1 || constructed != 2;
}

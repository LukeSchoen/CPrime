// EXPECT_EXIT: 0
int allocated, released;
void* operator new(__SIZE_TYPE__ size, int) { ++allocated; return ::operator new(size); }
void* operator new[](__SIZE_TYPE__ size, int) { ++allocated; return ::operator new[](size); }
void operator delete(void* pointer, int) { ++released; ::operator delete(pointer); }
void operator delete[](void* pointer, int) { ++released; ::operator delete[](pointer); }
int main() {
  void* scalar = ::operator new(8, 1);
  void* array = ::operator new[](16, 1);
  if (!scalar || !array || allocated != 2) return 1;
  ::operator delete(scalar, 1);
  ::operator delete[](array, 1);
  return released != 2;
}

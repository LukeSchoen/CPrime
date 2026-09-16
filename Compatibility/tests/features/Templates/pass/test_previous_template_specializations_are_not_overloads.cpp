// EXPECT_EXIT: 0
template<class T> int extent(const T &, int) { return sizeof(T); }
int extent(double, int) { return 99; }
int main() {
  char bytes[5] = {};
  char *pointer = bytes;
  if (extent(pointer, 0) != sizeof(pointer)) return 1;
  if (extent(bytes, 0) != sizeof(bytes)) return 2;
  if (extent("other text", 0) != sizeof("other text")) return 3;
  if (extent(pointer, 0) != sizeof(pointer)) return 4;
  return 0;
}

// EXPECT_EXIT: 0
struct Reader {
  int size() { return 99; }
  template<class T> int size() { return sizeof(T); }
  template<int A, int B> int value() { return A * 10 + B; }
};

template<class T> int read(T &reader) {
  if (reader.template value<1, 2>() != 12) return 1;
  if (reader.template value<3, 4>() != 34) return 2;
  if (reader.template value<1, 2>() != 12) return 3;
  if (reader.template size<char>() != 1) return 4;
  if (reader.template size<int>() != sizeof(int)) return 5;
  return reader.template size<char>() != 1;
}

int main() {
  Reader reader;
  if (read(reader)) return 1;
  if (reader.value<3, 4>() != 34 || reader.value<1, 2>() != 12) return 2;
  if (reader.size() != 99) return 3;
  return reader.size<char>() != 1;
}

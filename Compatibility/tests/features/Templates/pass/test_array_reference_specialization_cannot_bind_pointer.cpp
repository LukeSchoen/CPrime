template<class T> int extent(const T &value) { return sizeof(T); }
struct Word { int value; };
int operator+(const Word &a, const Word &b) { return 0; }
template<class T> int operator+(const Word &word, const T &value) { return sizeof(T); }
int main() {
  if (extent("ab") != 3) return 1;
  if (extent("abcd") != 5) return 2;
  const char *pointer = "value";
  if (extent(pointer) != sizeof(pointer)) return 3;
  Word word;
  if (word + "ab" != 3 || word + "abcd" != 5) return 4;
  if (word + pointer != sizeof(pointer)) return 5;
  return extent("ab") != 3;
}

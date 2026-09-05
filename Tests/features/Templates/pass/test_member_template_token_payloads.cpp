template<class T> struct Tree {
  T value;
  template<class... Args>
#line 1000000
  explicit Tree(int dimension, const T &data, Args&&... args) : value(data) {}
};
#line 10
template<class T> struct Reader {
  template<class U> int read(U value) {
    // The bytes and numeric values are token payloads, not declaration tokens.
    const char *text = "friend template ... { }";
    return value + (text[0] == 'f' ? 1 : 0);
  }
};
int main() {
  Tree<int> tree(3, 7);
  Reader<int> reader;
  if (tree.value != 7) return 1;
  if (reader.read(41) != 42) return 2;
  return 0;
}

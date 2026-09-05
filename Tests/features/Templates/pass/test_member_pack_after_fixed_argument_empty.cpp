template<class T> struct Buffer {
  int size;
  template<class... Args> void resize(int count, Args&&... args) { size = count; }
};
int main() {
  Buffer<int> b;
  b.resize(7);
  return b.size != 7;
}

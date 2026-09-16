namespace Library {
  template<class T> T *begin(T *p) { return p; }
  template<class T> T *end(T *p) { return p + 1; }
  template<class T> int difference(T *begin, T *end) { return end - begin; }
}
int main() {
  int values[3];
  return Library::difference(values, values + 3) != 3;
}

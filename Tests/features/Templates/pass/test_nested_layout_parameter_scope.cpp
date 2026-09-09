template<class T> struct Vector {
  T data[3];
  typedef Vector<T> Self;
};
template<class T> struct Box {
  typedef Vector<T> Vertex;
  void check(char (*)[sizeof(Vertex)]);
  Vertex values[2];
};
int main() { Box<int> box{}; return sizeof(box) != 6 * sizeof(int); }

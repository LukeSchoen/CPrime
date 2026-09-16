template<class T> struct Vector {
  T value;
  Vector() : value(0) {}
  Vector(const Vector<T>& source) { operator=(source); }
  Vector& operator=(const Vector<T>& source) { value = source.value; return *this; }
  inline ~Vector() {}
};
int main() { Vector<int> a; a.value = 9; Vector<int> b(a); return b.value != 9; }

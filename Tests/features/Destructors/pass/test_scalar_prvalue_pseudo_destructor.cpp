typedef int Scalar;
int calls;
Scalar value() { ++calls; return 7; }
template<class T> void destroy_value(T item) { item.~T(); }
int main() {
  value().~Scalar();
  destroy_value(11);
  return calls == 1 ? 0 : 1;
}

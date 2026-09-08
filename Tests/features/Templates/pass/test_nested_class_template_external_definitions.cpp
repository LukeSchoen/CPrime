template<class T> struct Outer { template<class U> struct Inner; };
template<class T> template<class U> struct Outer<T>::Inner {
  T first;
  U second;
  int sum() { return first + second; }
};
template<class T> template<class U> struct Outer<T>::Inner<U*> {
  T first;
  U *second;
  int sum() { return first + *second; }
};
int main() {
  Outer<int>::Inner<short> scalar = { 3, 4 };
  int value = 8;
  Outer<long>::Inner<int*> pointer = { 5, &value };
  return scalar.sum() != 7 || pointer.sum() != 13;
}

struct Other { long long number; };
template<class T> struct Store {
  T values[3];
  int unrelatedMemberSize() const { return sizeof(Other::number); }
};
template<class T> int bytes() { return sizeof(Store<T>::values); }

int main() {
  Store<double> value;
  if (bytes<int>() != 3 * sizeof(int)
      || bytes<double>() != 3 * sizeof(double)) return 1;
  if (value.unrelatedMemberSize() != sizeof(long long)) return 2;
  typedef decltype(Other::number) Number;
  typedef decltype(Store<int>::values) Array;
  if (sizeof(Number) != sizeof(long long) || sizeof(Array) != 3 * sizeof(int)) return 3;
  return 0;
}

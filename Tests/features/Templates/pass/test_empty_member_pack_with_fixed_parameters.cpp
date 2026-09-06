template<class T> struct Store {
  int count = 0;
  template<class... Args> void append(Args... args);
  template<class... Args> void insert(int index, Args... args) {
    count = index + sizeof...(args);
  }
};
template<class T> template<class... Args>
void Store<T>::append(Args... args) {
  insert(count, args...);
  ++count;
}
int main() {
  Store<double> first;
  first.append();
  if (first.count != 1) return 1;
  first.append(2, 3);
  if (first.count != 4) return 2;
  first.insert(8);
  if (first.count != 8) return 3;
  Store<int> second;
  second.append(1, 2, 3);
  second.append();
  if (second.count != 5) return 4;
  second.insert(7);
  return second.count != 7;
}

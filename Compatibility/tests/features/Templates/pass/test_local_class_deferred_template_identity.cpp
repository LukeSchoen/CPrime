template<class T> struct Store {
  T value;
  void set(const T &source);
  void set(T &&source);
  int read() const;
};
template<class T> void Store<T>::set(const T &source) { value = source; }
template<class T> void Store<T>::set(T &&source) { value = source; }
template<class T> int Store<T>::read() const { return value.number; }
int first() {
  struct Item { int number; };
  Store<Item> store;
  Item value = {7};
  store.set(value);
  return store.read();
}
int second() {
  struct Item { long long padding; int number; };
  Store<Item> store;
  Item value = {99, 13};
  store.set(value);
  return store.read();
}
int main() { return first() != 7 || second() != 13; }

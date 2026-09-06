template<class T> struct Subscription;
template<class T> struct Event {
  using ID = long long;
  using Token = Subscription<T>;
  using Later = int;
  T value;
};
template<class T> struct Subscription {
  using ID = typename Event<T>::ID;
  using Other = typename Event<T>::Later;
  ID id;
};
int main() { Event<int>::Token t = {9}; return t.id != 9; }

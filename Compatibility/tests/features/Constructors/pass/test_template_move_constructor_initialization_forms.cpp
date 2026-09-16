#include <new>

template<class T> struct Box {
  int value;
  Box(int n) : value(n) {}
  Box(const Box<T> &other) : value(other.value + 1) {}
  Box(Box<T> &&other) : value(other.value + 5) { other.value = -1; }
};

int main() {
  Box<int> first(10);
  Box<int> direct(static_cast<Box<int>&&>(first));
  if (direct.value != 15 || first.value != -1) return 1;
  Box<short> second(20);
  Box<short> copy = static_cast<Box<short>&&>(second);
  if (copy.value != 25 || second.value != -1) return 2;
  Box<char> third(30);
  Box<char> functional = Box<char>(static_cast<Box<char>&&>(third));
  if (functional.value != 35 || third.value != -1) return 3;
  Box<float> fourth(40);
  union Storage { char bytes[sizeof(Box<float>)]; long long alignment; } storage;
  Box<float> *placed = new (storage.bytes) Box<float>(static_cast<Box<float>&&>(fourth));
  if (placed->value != 45 || fourth.value != -1) return 4;
  Box<int> lvalueCopy(direct);
  return lvalueCopy.value != 16 || direct.value != 15;
}

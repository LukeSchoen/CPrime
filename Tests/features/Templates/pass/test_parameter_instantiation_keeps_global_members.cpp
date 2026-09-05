template<class T> struct Bag {
  int erase(int);
  int erase(int, int);
  int erase(const Bag<long>&);
};
int accepts(Bag<char>);
template<class T> int Bag<T>::erase(int x) { return x + 1; }
template<class T> int Bag<T>::erase(int x, int y) { return x + y; }
template<class T> int Bag<T>::erase(const Bag<long>& indexes) { return 19; }
int main() {
  Bag<char> b; Bag<long> indexes;
  return b.erase(2) != 3 || b.erase(4, 5) != 9 || b.erase(indexes) != 19;
}

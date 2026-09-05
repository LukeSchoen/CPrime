template<class T> struct Table {
  int value;
  static int create();
  Table();
};
template<class T> int Table<T>::create() { return 7; }
template<class T> Table<T>::Table() : value(create()) {}
int main() { Table<int> t; return t.value != 7; }

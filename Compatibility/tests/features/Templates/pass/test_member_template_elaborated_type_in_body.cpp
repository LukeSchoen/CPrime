struct Item { int value; };
struct Factory {
  template<class T> int read(T value) {
    struct Item item{value};
    return item.value;
  }
};
int main() { Factory factory; return factory.read(7) != 7; }

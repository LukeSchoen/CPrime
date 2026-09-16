struct A {};

struct RefA {
  A *pointer;
  int copied;
  RefA(A *value) : pointer(value), copied(0) {}
  RefA(const RefA &other) : pointer(other.pointer), copied(1) {}
};

struct AccRefA {
  RefA value;
  AccRefA() : value((A *)0) {}
  operator RefA &() { return value; }
};

int main() {
  AccRefA source;
  RefA result = source;
  return !result.copied;
}

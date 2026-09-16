template <typename T>
struct Enclosing {
  struct Iterator {
    Enclosing owner;

    friend int inspect(Iterator value) {
      return value.owner.marker;
    }
  };

  int marker;

  Enclosing() : marker(37) {}
};

int main() {
  Enclosing<int>::Iterator value;
  return inspect(value) == 37 ? 0 : 1;
}

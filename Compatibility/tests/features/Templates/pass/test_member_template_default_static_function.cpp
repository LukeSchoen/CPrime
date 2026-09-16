struct Finder {
  template <typename T>
  static bool accept(const T&) { return true; }

  template <typename T>
  int find(const T& value, bool (*predicate)(const T&) = accept) const {
    return predicate(value) ? 37 : 0;
  }
};

int main() {
  Finder finder;
  return finder.find(finder) == 37 ? 0 : 1;
}

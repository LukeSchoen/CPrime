// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
int nested_lifetimes;
enum Kind : long long { zero, one, two };
struct Container {
  struct Entry {
    long long padding;
    Kind kind;
    Entry() { ++nested_lifetimes; }
    ~Entry() { --nested_lifetimes; }
  };
  union Choice { long long wide; char byte; };
  struct Later;
  long long value;
  bool match(Kind kind) { return kind == two; }
  bool read(Kind channel);
};
bool Container::read(Kind channel) {
  Kind kind = Kind(channel + 2);
  return match(kind);
}
struct Empty { struct Nested { char bytes[32]; }; };
struct Anonymous { union { int number; float fraction; }; };
int main() {
  static_assert(sizeof(Container) == sizeof(long long), "nested types occupy no storage");
  static_assert(sizeof(Empty) == 1, "nested declarations leave an empty class");
  static_assert(sizeof(Anonymous) == sizeof(int), "anonymous unions still occupy storage");
  {
    Container container = {};
    if (nested_lifetimes != 0 || !container.read(zero)) return 1;
    Container::Entry entry;
    if (nested_lifetimes != 1) return 2;
    Anonymous value; value.number = 7;
    if (value.number != 7) return 4;
  }
  return nested_lifetimes;
}

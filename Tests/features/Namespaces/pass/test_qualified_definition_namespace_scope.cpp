namespace Api {
  enum class Kind { first = 3, second = 5 };
  struct Value { int n; };
  int offset = 7;
  int read(Kind kind, Value value);
  inline int deferred(Kind kind);
}
int Api::read(Kind kind, Value value) {
  return (kind == Kind::first ? value.n : 0) + offset;
}
inline int Api::deferred(Kind kind) { Value value = { 4 }; return read(kind, value); }
int main() {
  Api::Value value = { 10 };
  return Api::read(Api::Kind::first, value) != 17 || Api::deferred(Api::Kind::first) != 11;
}

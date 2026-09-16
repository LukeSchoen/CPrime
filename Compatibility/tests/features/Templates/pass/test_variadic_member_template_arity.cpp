// A member function template that owns a parameter pack still requires the
// parameters that precede the pack. Value-initialization of a class with such
// a constructor template used to replay the pack-owning constructor for zero
// arguments and fail with "unresolved member template argument for native
// linkage" instead of selecting the default constructor.

struct Widget
{
  Widget() : value(7) {}
  template<class F, class... Rest>
  explicit Widget(F &&first, Rest &&...rest) : value(1) {}
  int value;
};

struct Sink
{
  int Call() { return 4; }
  template<class F, class... Rest>
  int Call(F &&first, Rest &&...rest) { return 5; }
};

int main()
{
  Widget empty = Widget();
  if (empty.value != 7) return 1;
  Widget one(3);
  if (one.value != 1) return 2;
  Widget many(3, 4, 5);
  if (many.value != 1) return 3;

  Sink sink;
  if (sink.Call() != 4) return 4;
  if (sink.Call(1) != 5) return 5;
  if (sink.Call(1, 2, 3) != 5) return 6;
  return 0;
}

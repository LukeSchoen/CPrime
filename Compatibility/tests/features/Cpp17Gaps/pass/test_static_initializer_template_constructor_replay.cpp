// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: replayed_constructor_body.  A template constructor whose body
// uses `++` on its iterator parameter is queued when the template is
// instantiated.  Compiling that body for real code while a later
// namespace-scope static initializer is folded must not read the body's
// runtime operations as constant evaluation.
template <class T>
struct box
{
  template <class It>
  box(It first, It last)
  {
    for (; first != last; ++first) { }
  }
};

static box<int> make_box(const int *first, const int *last)
{
  return box<int>(first, last);
}

struct tag
{
};

static const tag value = tag();

int main()
{
  int data[3] = {1, 2, 3};
  box<int> b = make_box(data, data + 3);
  (void)b;
  return 0;
}

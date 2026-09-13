// A partial specialization's constructor may name a typedef inherited through
// its dependent base.  The initializer must resolve that typedef after the
// outer arguments are bound.
template <typename T> struct Holder {
  typedef Holder type;
  int value;
  Holder(int input) : value(input) {}
};

template <typename T, typename U = int> struct Wrapper;

template <typename T> struct Wrapper<T> : Holder<T> {
  Wrapper() : Holder<T>::type(37) {}
};

int main()
{
  Wrapper<float> value;
  return value.value == 37 ? 0 : 1;
}

// A bound template-template parameter used in an elaborated type must be
// substituted with the concrete class template argument when the class
// template body is replayed.  Otherwise `friend class F<int>;` replayed as an
// elaborated-type lookup of the parameter token itself and failed with
// "class template expected in elaborated type" once the holder was
// instantiated.

template <typename T> struct data {
  T value;
};

template <template <typename> class F> struct holder {
  friend class F<int>;
  data<int> something;
};

int main() {
  holder<data> h;
  h.something.value = 7;
  return h.something.value == 7 ? 0 : 1;
}

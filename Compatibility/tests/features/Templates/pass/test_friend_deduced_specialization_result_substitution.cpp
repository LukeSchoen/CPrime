// EXPECT_COMPILE_ARGS: -Werror

/* A friend specialization declaration inside a class template deduces its
   template arguments from the parameters it spells and names the specialization
   through the result type it declares.  Replayed during the explicit
   instantiation of the class, the primary's instance has no symbol yet, so the
   written signature has to be matched against the substituted parameter list
   and declared result type. */
struct stream { int value; };

template <class T> class holder;
template <class T> stream& take(stream& source, holder<T>& value);

template <class T> class holder {
public:
  friend stream& take<>(stream&, holder<T>&);
private:
  T member;
};

template <> stream& take<int>(stream& source, holder<int>& value);
template class holder<int>;

template <> stream& take<int>(stream& source, holder<int>& value) {
  value.member = 7;
  return source;
}

int main() {
  stream target;
  holder<int> value;
  target.value = 0;
  take(target, value);
  return value.member == 7 ? 0 : 1;
}

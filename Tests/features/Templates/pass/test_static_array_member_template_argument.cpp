/* A static data member of array type used as a non-type template argument
   decays to a pointer, both when spelled through the enclosing class and when
   spelled unqualified inside it. */
template<const char *Text> struct Holder {
  static const char *text;
};
template<const char *Text> const char *Holder<Text>::text = Text;

struct Owner {
  static const char text[4];
  typedef Holder<text> Alias;
};
const char Owner::text[4] = "abc";

template<class T> struct Wrap {
  static const char text[4];
  typedef Holder<Wrap<T>::text> Alias;
};
template<class T> const char Wrap<T>::text[4] = "xyz";

int main() {
  if (!(Holder<Owner::text>::text == Owner::text)) return 1;
  if (!(Wrap<int>::Alias::text == Wrap<int>::text)) return 2;
  if (!(Wrap<float>::Alias::text == Wrap<float>::text)) return 3;
  if (!(Wrap<int>::Alias::text != Wrap<float>::text)) return 4;
  return 0;
}

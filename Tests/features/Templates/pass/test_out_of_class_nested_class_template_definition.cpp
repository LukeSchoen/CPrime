// A nested class template declared inside a class and defined out of class
// must complete that declaration.  The definition's qualifier used to be
// registered as a class template itself, which hid the nested name.

// Non-template outer class, plain out-of-class definition.
struct Outer
{
  template <class T> struct Inner;
};

template <class T> struct Outer::Inner
{
  int value;
};

Outer::Inner<int> inner_object;

// Out-of-class definition carrying a default template argument.
struct Defaulted
{
  template <typename T> struct Member;
};

template <typename T = int> struct Defaulted::Member
{
  int value;
};

Defaulted::Member<> defaulted_object;

// Nested class template of a class template completed out of class with a
// base-clause list.
template <typename T> struct Holder
{
  int payload;
  template <int> struct Nested;
};

template <typename T> template <int I> struct Holder<T>::Nested : Holder<T>
{
  int extra;
};

Holder<int>::Nested<42> nested_object;

int main ()
{
  inner_object.value = 1;
  defaulted_object.value = 2;
  nested_object.extra = 3;
  nested_object.payload = 4;
  return inner_object.value + defaulted_object.value
         + nested_object.extra + nested_object.payload == 10 ? 0 : 1;
}

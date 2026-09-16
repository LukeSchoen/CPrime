// Attribute, cleanup-order and layout extensions.

namespace empty_attribute
{
void Dummy() __attribute__(( , ));
void Dummy() {}
}

namespace aligned_typedef_deduction
{
template <int> struct A;

template <> struct A<0>
{
  typedef int X __attribute((aligned(4)));
};

template <typename T> void foo(const A<0>::X &, T);

void bar()
{
  foo(A<0>::X(), 0);
}
}

namespace declarator_attributes
{
typedef struct S
{
  int x;
} T;

T(foo)(T x);
T __attribute__((unused)) bar(T x);
struct S(__attribute__((unused)) baz)(T x);
T(__attribute__((unused)) qux)(T x);

struct U
{
  U(__attribute__((unused)) int) {}
  U(__attribute__((unused)) corge)(int) { return U(0); }
};

T foo(T x) { return x; }
T bar(T x) { return x; }
T baz(T x) { return x; }
T qux(T x) { return x; }

int check()
{
  T a;
  a.x = 1;
  a = foo(a);
  a = bar(a);
  a = baz(a);
  a = qux(a);
  U u(5);
  U v = u.corge(3);
  (void)v;
  return a.x == 1 ? 0 : 1;
}
}

namespace packed_enum
{
enum XXX
{
  xyzzy = 3
} __attribute__((packed));

int check()
{
  return sizeof(xyzzy) == 1 ? 0 : 1;
}
}

int cleanup_state;

struct CleanupOrderObject
{
  ~CleanupOrderObject()
  {
    cleanup_state = cleanup_state == 1 ? 2 : -1;
  }
};

void cleanup(void *)
{
  cleanup_state = cleanup_state == 0 ? 1 : -1;
}

int cleanup_order_check()
{
  cleanup_state = 0;
  {
    CleanupOrderObject object __attribute__((cleanup(cleanup)));
    (void)object;
  }
  return cleanup_state == 2 ? 0 : 1;
}

namespace extern_template_static_const
{
template <typename _CharT> class basic_string;
typedef basic_string<char> string;

template <typename _CharT> struct basic_string
{
  static const int npos = -1;
};

template <typename _CharT> const int basic_string<_CharT>::npos;
extern template class basic_string<char>;

struct A
{
  static const long npos = string::npos;
};

int check()
{
  return A::npos == -1 ? 0 : 1;
}
}

namespace dependent_array_bound
{
template <class T> struct Outer
{
  struct Inner
  {
    struct
    {
      int extent() { return 1; }
    } b;
  };

  void test();
};

template <class T> void Outer<T>::test()
{
  Inner a;
  int vla[a.b.extent()];
  vla[0] = 0;
}

int check()
{
  Outer<char> a;
  a.test();
  return 0;
}
}

int main()
{
  if (int code = declarator_attributes::check()) { return code; }
  if (int code = packed_enum::check()) { return code; }
  if (int code = cleanup_order_check()) { return code; }
  if (int code = extern_template_static_const::check()) { return code; }
  return dependent_array_bound::check();
}

template <bool B, class T1, class T2> struct if_c_local
{
  typedef T1 type;
};

template <class T1, class T2> struct if_c_local<false, T1, T2>
{
  typedef T2 type;
};

template <class T1, class T2, class T3> struct if_local
{
private:
  typedef if_c_local<static_cast<bool>(T1::value), T2, T3> almost_type_;
public:
  typedef typename almost_type_::type type;
};

template <class C, class F1, class F2>
struct eval_if_local : if_local<C, F1, F2>::type
{
};

template <class T> struct identity
{
  typedef T type;
};

struct flag_local
{
  enum { value = 0 };
};

template <class T> struct wrapper
{
  struct condition : flag_local {};
  typedef typename eval_if_local<condition, identity<T>, identity<int> >::type selected;
};

wrapper<double>::selected value;

int main()
{
  return 0;
}

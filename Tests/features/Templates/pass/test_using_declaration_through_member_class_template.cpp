struct null_type;

template <typename T1, typename T2>
struct tuple_impl
{
  template <typename U>
  struct append
  {
    typedef tuple_impl<U, null_type> type;
  };

  int data;
};

template <typename T1>
struct tuple : tuple_impl<T1, null_type>::template append<T1>::type
{
  using tuple_impl<T1, null_type>::template append<T1>::type::data;
};

int main()
{
  tuple<int> value;
  value.data = 7;
  return value.data == 7 ? 0 : 1;
}

// EXPECT_EXIT: 0

struct tag1
{
};

struct tag2
{
};

template <class T>
T (changesign_impl)(T x, tag1 const &)
{
  return -x;
}

template <class T>
T (changesign_impl)(T x, tag2 const &)
{
  return -x;
}

template <class T>
T changesign_impl(T x, tag1 const &);

int main()
{
  return 0;
}

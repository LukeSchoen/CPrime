// EXPECT_EXIT: 0
struct Source
{
  template<class T>
  operator T();
};

template<class T>
Source::operator T()
{
  return T();
}

int main()
{
  Source source;
  int value = source.operator int();
  return value != 0;
}

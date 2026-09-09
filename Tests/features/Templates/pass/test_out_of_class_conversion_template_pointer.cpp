// EXPECT_EXIT: 0
struct Source
{
  template<class T>
  operator T*();
};

template<class T>
Source::operator T*()
{
  return 0;
}

int main()
{
  Source source;
  char *value = source.operator char*();
  return value != 0;
}

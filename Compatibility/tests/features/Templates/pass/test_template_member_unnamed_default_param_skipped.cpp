struct SkipDefaultedMemberTemplate
{
  int value;

  template<typename T, typename = T>
  explicit SkipDefaultedMemberTemplate(const T &next)
  {
    value = 1;
  }

  SkipDefaultedMemberTemplate() : value(0)
  {
  }
};

int main()
{
  SkipDefaultedMemberTemplate item;
  return item.value;
}

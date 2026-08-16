namespace std
{
  template<typename T> T declval();
}

int ToInt(int value);

#define TypeOf(value) decltype(value)
#define TypeInstance(type) std::declval<type>()
#define RequiresToInt(T) typename = TypeOf(ToInt(TypeInstance(T)))

struct SkipDecltypeDefaultMemberTemplate
{
  int value;

  template<typename T, RequiresToInt(T)> explicit SkipDecltypeDefaultMemberTemplate(const T &next)
  {
    value = 1;
  }

  SkipDecltypeDefaultMemberTemplate() : value(0)
  {
  }
};

int main()
{
  SkipDecltypeDefaultMemberTemplate item;
  return item.value;
}

template<typename T>
T declval();

class DeferredBodyString;
DeferredBodyString ToString(int value);

class DeferredBodyString
{
public:
  DeferredBodyString();
  DeferredBodyString(const DeferredBodyString &other);
  DeferredBodyString(const char *text);

  template<typename T, typename = decltype(ToString(declval<T>()))>
  explicit DeferredBodyString(const T &value)
  {
  }

  DeferredBodyString &operator+=(const DeferredBodyString &rhs);
};

template<typename T, typename = decltype(ToString(declval<T>()))>
DeferredBodyString operator+(const T &lhs, const DeferredBodyString &rhs)
{
  DeferredBodyString result = DeferredBodyString(lhs);
  result += rhs;
  return result;
}

int main()
{
  return 0;
}

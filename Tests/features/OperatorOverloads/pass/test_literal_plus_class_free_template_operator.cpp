struct Text
{
  int length;

  Text() : length(0) {}
  Text(const Text &rhs) : length(rhs.length) {}
  explicit Text(const char *value) : length(value ? 1 : 0) {}

  Text& operator+=(const Text &rhs)
  {
    length += rhs.length;
    return *this;
  }
};

Text operator+(const Text &lhs, const Text &rhs)
{
  Text result(lhs);
  result += rhs;
  return result;
}

template<typename T> Text operator+(const Text &lhs, const T &rhs)
{
  Text result(lhs);
  result += Text(rhs);
  return result;
}

template<typename T> Text operator+(const T &lhs, const Text &rhs)
{
  Text result(lhs);
  result += rhs;
  return result;
}

int main()
{
  Text name("channel");
  Text message = "No data has been bound for " + name + ": value";
  (void)message;
  return 0;
}

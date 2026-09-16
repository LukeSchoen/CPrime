namespace std
{
  template<typename T>
  T&& declval();
}

class DeclvalString;

DeclvalString clToString(int value);

class DeclvalString
{
public:
  template<typename T, typename = decltype(clToString(std::declval<T>()))>
  explicit DeclvalString(const T &value)
  {
  }
};

template<typename T, typename = decltype(clToString(std::declval<T>()))>
DeclvalString operator+(const T &lhs, const DeclvalString &rhs)
{
  return DeclvalString(lhs);
}

int main()
{
  return 0;
}

template <typename T>
T mini_min(const T &value)
{
  return value;
}

template <typename T, typename... Args>
T mini_min(const T &value, Args... args)
{
  T min_args = mini_min(args...);
  return value < min_args ? value : min_args;
}

int main()
{
  long long a = 9;
  long long b = 4;
  long long c = 6;
  return mini_min(a, b, c) == 4 ? 0 : 1;
}

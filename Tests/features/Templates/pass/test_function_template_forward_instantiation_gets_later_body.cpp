template<typename T, typename U>
int delayed_sum(T first, U second);

typedef decltype(delayed_sum(1, 2)) DelayedResult;

int call_delayed_sum()
{
  return delayed_sum(19, 23);
}

template<typename T, typename U>
int delayed_sum(T lhs, U rhs)
{
  return lhs + rhs;
}

int main()
{
  DelayedResult value = call_delayed_sum();
  return value == 42 ? 0 : 1;
}

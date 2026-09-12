#ifndef CPRIME_TEST_VARIADIC_STATIC_MEMBER_H
#define CPRIME_TEST_VARIADIC_STATIC_MEMBER_H

struct VariadicResult
{
  int value;
};

struct VariadicFactory
{
  template <typename... Args>
  static VariadicResult Make(Args &&...args);
};

template <typename... Args>
VariadicResult VariadicFactory::Make(Args &&...args)
{
  int values[] = {(int)args...};
  VariadicResult result = {values[0]};
  return result;
}

#endif

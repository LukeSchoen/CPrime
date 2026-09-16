// Consolidated from similar standalone regressions; each cpc_case_N preserves one.
#include <initializer_list>

namespace cpc_case_0
{

enum BraceKey
{
  BraceKeyA,
  BraceKeyB
};

int run()
{
  (void)std::initializer_list<BraceKey>{ BraceKeyA, BraceKeyB };
  return 0;
}
}

namespace cpc_case_1
{

enum Key
{
  KeyA,
  KeyB
};

template <typename T>
struct List
{
  List() {}
  List(const std::initializer_list<T> &values)
  {
    (void)values.begin();
  }
};

static void materialize_lists()
{
  List<Key> one(std::initializer_list<Key>{ KeyA, KeyB });
  List<Key> two(std::initializer_list<Key>{ KeyB, KeyA });
  (void)one;
  (void)two;
}

int run()
{
  return 0;
}
}

namespace cpc_case_2
{

enum KeyType
{
  KeyTypeA,
  KeyTypeB
};

int run()
{
  std::initializer_list<KeyType> values = { KeyTypeA, KeyTypeB };
  (void)values;
  return 0;
}
}

namespace cpc_case_3
{
enum class Value { first = 3, second = 7, third = 11 };
int sum(std::initializer_list<Value> values) {
  int result = 0;
  for (const Value *p = values.begin(); p != values.end(); ++p) result += (int)*p;
  return result;
}
int run() {
  std::initializer_list<Value> values = { Value::first, Value::second, Value::third };
  if (values.size() != 3 || sum(values) != 21) return 1;
  if (sum(std::initializer_list<Value>{ Value::second, Value::first }) != 10) return 2;
  if (sum(values) != 21) return 3;
  std::initializer_list<Value> empty;
  return empty.size() != 0 || empty.begin() != empty.end();
}
}

int main()
{
  if (int code = cpc_case_0::run()) { return code; }
  if (int code = cpc_case_1::run()) { return code; }
  if (int code = cpc_case_2::run()) { return code; }
  if (int code = cpc_case_3::run()) { return code; }
  return 0;
}

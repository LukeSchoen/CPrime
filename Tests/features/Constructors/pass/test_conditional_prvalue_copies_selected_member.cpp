#include <type_traits>

int alive, copies, moves, made, conditions;
struct Value {
  int value;
  Value(int n) : value(n) { ++alive; }
  Value(const Value& other) : value(other.value) { ++alive; ++copies; }
  Value(Value&& other) : value(other.value) { other.value = -1; ++alive; ++moves; }
  ~Value() { --alive; }
};
struct Holder {
  Value member;
  Holder(int n) : member(n) {}
  Value get() const { ++made; return member; }
  Value choose(const Holder& other, bool first) const
  {
    return first ? other.get() : member;
  }
  Value reverse(const Holder& other, bool first) const
  {
    return first ? member : other.get();
  }
};
bool condition(bool value) { ++conditions; return value; }

int exercise(bool flag)
{
  Holder source(7), other(13);
  static_assert(std::is_same<decltype(flag ? other.get() : source.member), Value>::value,
                "mixed class operands produce a prvalue");
  const Value& constant = source.member;
  static_assert(std::is_same<decltype(flag ? other.get() : constant), const Value>::value,
                "conditional preserves the common const qualification");
  int before = copies;
  {
    Value selected = source.choose(other, flag);
    if (selected.value != (flag ? 13 : 7) || alive != 3) return 1;
  }
  {
    Value selected = source.reverse(other, flag);
    if (selected.value != (flag ? 7 : 13) || alive != 3) return 2;
  }
  {
    const Value& selected = condition(flag) ? other.get() : source.member;
    if (selected.value != (flag ? 13 : 7) || alive != 3) return 3;
  }
  if (source.member.value != 7 || other.member.value != 13) return 4;
  if (copies != before + 3 || moves || alive != 2) return 5;
  return 0;
}
int main()
{
  int result = exercise(false);
  if (result) return result;
  if (alive) return 6;
  result = exercise(true);
  if (result) return result + 10;
  return alive || made != 3 || conditions != 2;
}

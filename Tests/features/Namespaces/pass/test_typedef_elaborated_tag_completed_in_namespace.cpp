// EXPECT_EXIT: 0
// A tag named without a body inside a namespace - an elaborated type
// specifier such as `typedef struct Tag Alias;` - has to belong to the
// namespace member, so the later `struct Tag { ... }` completes that same tag
// instead of declaring a second one that stays incomplete.  LIBZPAQ.cpp binds
// its trbudget_t helper this way.
namespace ns {
  typedef struct Tag Alias;
  struct Tag { int value; };
  static void Set(Alias *alias) { alias->value = 7; }
}

int main()
{
  ns::Alias alias;
  ns::Set(&alias);
  return alias.value == 7 ? 0 : 1;
}

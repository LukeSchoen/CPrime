// EXPECT_COMPILE_ARGS: -Iinclude/runtime -Ithird-party/win32-sdk/include
#include <typeinfo>

struct Flags {
  enum Value { no, yes, root };

  template<Value V> struct Select {
    static Value value() { return no; }
  };
};

template<> struct Flags::Select<Flags::yes> {
  static Flags::Value value() { return Flags::yes; }
};

int main() {
  Flags::Select<Flags::yes> selected;
  const std::type_info& identity = typeid(selected);
  return Flags::Select<Flags::no>::value() != Flags::no
      || selected.value() != Flags::yes
      || !identity.name();
}

/* GCC's system-header mode accepts several legacy declarations without
   extending those forms to ordinary source. */
# 1 "test_system_header_legacy_members_and_literals.h" 1 3
struct Source {
  Source(int) : value() {}
  untyped() {}
  int value;
};

struct Target {
  Target(Source source) : value(source.value) {}
  int value;
};

struct Holder {
  Holder(char *) : pointer() {}
  *pointer;
};

struct Legacy {
  Legacy(char *) : value(1) {}
  int value;
};

struct Wrapper {
  Wrapper(Legacy legacy) : value(legacy.value) {}
  int value;
};

struct Operators {
  operator+=(int) {}
};

Wrapper header_wrapper = Wrapper("");
# 2 "test_system_header_legacy_members_and_literals.cpp" 2

int main()
{
  Target target(7);
  Operators operators;
  operators += 1;
  return target.value != 0 || header_wrapper.value != 1;
}

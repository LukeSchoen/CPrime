// EXPECT_EXIT: 0
struct primary { virtual ~primary() {} };
struct secondary { virtual void call() = 0; };
struct derived : primary, secondary {
  void call() { seen = this; }
  static derived *seen;
};

derived *derived::seen;
void (derived::*from_secondary)() = &secondary::call;
void (derived::*from_derived)() = &derived::call;

int main()
{
  derived value;
  (value.*from_secondary)();
  if (derived::seen != &value) return 1;
  derived::seen = 0;
  (value.*from_derived)();
  return derived::seen == &value ? 0 : 2;
}

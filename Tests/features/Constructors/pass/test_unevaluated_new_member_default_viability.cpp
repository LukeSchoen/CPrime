// EXPECT_COMPILE_ONLY: 1
struct MissingConst { const int value; };
struct MissingReference { int &value; };
struct Valid { int value; };

template<class T> struct probe {
  template<int> struct size {};
  typedef char yes;
  struct no { char value[2]; };
  template<class U> static yes test(size<sizeof(new U)> *);
  template<class U> static no test(...);
  static const bool value = sizeof(test<T>(0)) == sizeof(yes);
};

static_assert(!probe<MissingConst>::value, "const member needs an initializer");
static_assert(!probe<MissingReference>::value, "reference member needs an initializer");
static_assert(probe<Valid>::value, "ordinary member is default-initializable");

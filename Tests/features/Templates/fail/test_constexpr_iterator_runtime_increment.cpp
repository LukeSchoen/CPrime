// EXPECT_COMPILE_FAIL: 1
struct Iterator {
  int value;
  Iterator &operator++() { ++value; return *this; }
};
constexpr int bad() { Iterator iterator{0}; (++iterator, 0); return 7; }
static_assert(bad() == 7);

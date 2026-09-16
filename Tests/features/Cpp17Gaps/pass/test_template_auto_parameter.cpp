// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: template_auto. `template<auto>` and `template<auto...>`
// accept value, character and boolean arguments, including an empty pack, with
// no retained case before this one.

template <auto Value> struct box {
  static constexpr auto value = Value;
};

template <auto... Values> struct count {
  static constexpr int value = sizeof...(Values);
};

static_assert(box<5>::value == 5);
static_assert(box<'a'>::value == 'a');
static_assert(count<1, 2, 3>::value == 3);
static_assert(count<>::value == 0);

int main()
{
  return box<true>::value ? 0 : 1;
}

// EXPECT_EXIT: 0

template<int Index>
struct placeholder_arg
{
  constexpr placeholder_arg()
  {
  }
};

template<int Index>
constexpr bool operator==(placeholder_arg<Index> const &,
                          placeholder_arg<Index> const &)
{
  return true;
}

namespace placeholders
{

static constexpr placeholder_arg<1> _1;

}

int main()
{
  return placeholders::_1 == placeholders::_1 ? 0 : 1;
}

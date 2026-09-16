// EXPECT_EXIT: 0

struct na
{
};

template <class Dummy = na>
struct vector0;

template <class V, int N>
struct v_iter
{
};

template <>
struct vector0<na>
{
  typedef v_iter<vector0<>, 0> begin;
  typedef v_iter<vector0<>, 0> end;
};

int main()
{
  return 0;
}

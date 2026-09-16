// EXPECT_EXIT: 0

template<typename T>
struct Pair
{
  T x;
  T y;
};

typedef Pair<float> value2;

int main()
{
  int first = 4, value2 = 0;
  value2 = first + 3;
  return value2 == 7 ? 0 : 1;
}

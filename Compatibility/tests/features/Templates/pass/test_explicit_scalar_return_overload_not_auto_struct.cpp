// EXPECT_EXIT: 0

template<typename T>
class Pair
{
public:
  T x;
  T y;
};

template<typename T>
T clampValue(const T &value, const T &low, const T &high)
{
  return value < low ? low : (value > high ? high : value);
}

template<typename T>
auto clampValue(const Pair<T> &value, const Pair<T> &low,
                const Pair<T> &high)
{
  Pair<T> result;
  result.x = clampValue(value.x, low.x, high.x);
  result.y = clampValue(value.y, low.y, high.y);
  return result;
}

int main()
{
  return clampValue(float(5), 0.0f, 3.0f) == 3.0f ? 0 : 1;
}

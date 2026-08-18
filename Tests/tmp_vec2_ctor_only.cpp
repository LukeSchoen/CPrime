template <typename T>
struct Vec2ForCtor
{
  T x;
  T y;

  Vec2ForCtor() = default;
  Vec2ForCtor(const T &_x, const T &_y) : x(_x), y(_y) {}
};

typedef Vec2ForCtor<double> Vec2DForCtor;

int main()
{
  Vec2DForCtor xy(1.0, 2.0);
  (void)xy;
  return 0;
}

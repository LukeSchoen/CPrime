// EXPECT_EXIT: 0

class Vec2
{
public:
  float x;
  float y;

  Vec2() = default;
  Vec2(const float &v) : x(v), y(v) {}
  Vec2(const float &vx, const float &vy) : x(vx), y(vy) {}
};

int main()
{
  Vec2 v = Vec2({ -5.0f, -17.0f });
  return v.x == -5.0f && v.y == -17.0f ? 0 : 1;
}

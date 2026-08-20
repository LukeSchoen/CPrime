// EXPECT_EXIT: 0

int id(int value)
{
  return value;
}

struct Vec2
{
  int x;
  int y;

  int LengthSquared() const;
  int Length() const
  {
    return id(LengthSquared());
  }
};

int Vec2::LengthSquared() const
{
  return x * x + y * y;
}

int main()
{
  Vec2 v;
  v.x = 3;
  v.y = 4;
  return id(v.LengthSquared()) == 25 && v.Length() == 25 ? 0 : 1;
}

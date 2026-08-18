struct Vec2
{
  float x;
  float y;

  Vec2(float xIn, float yIn) : x(xIn), y(yIn) {}
};

static void construct_vec_from_single_braced_argument()
{
  Vec2 value(0.0f, 0.0f);
  value = Vec2({ -5.0f, -17.0f });
  (void)value;
}

int main()
{
  return 0;
}

struct Vec2
{
  float x;
  float y;

  Vec2(float xIn, float yIn) : x(xIn), y(yIn) {}
};

struct Holder
{
  Vec2 pos;

  Holder() : pos(0.0f, 0.0f) {}
};

static void assign_member_from_braced_functional_constructor()
{
  Holder holder;
  holder.pos = Vec2({ -5.0f, -17.0f });
}

int main()
{
  return 0;
}

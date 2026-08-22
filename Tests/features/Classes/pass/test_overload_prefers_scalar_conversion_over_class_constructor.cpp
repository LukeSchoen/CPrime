// EXPECT_EXIT: 0

class Vec2
{
public:
  float x;
  Vec2(float value = 0) { x = value; }
};

class Image {};

class Draw
{
public:
  int picked;
  void Load(const Image &image, Vec2 hotSpot, float maxSize);
  void Load(const Image &image, Vec2 hotSpot, Vec2 size2d);
};

void Draw::Load(const Image &image, Vec2 hotSpot, float maxSize)
{
  picked = 1;
}

void Draw::Load(const Image &image, Vec2 hotSpot, Vec2 size2d)
{
  picked = 2;
}

int main()
{
  Image image;
  Draw draw;
  draw.Load(image, Vec2(0.5f), 64);
  return draw.picked == 1 ? 0 : 1;
}

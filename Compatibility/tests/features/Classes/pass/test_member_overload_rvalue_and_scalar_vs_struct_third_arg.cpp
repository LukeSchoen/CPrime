struct LoadImage
{
  LoadImage() {}
  LoadImage(int) {}
};

struct LoadVec
{
  float x;
  float y;
  LoadVec(float v) : x(v), y(v) {}
};

struct LoadTarget
{
  void load(const LoadImage &image, LoadVec hotSpot, float maxSize)
  {
    (void)image; (void)hotSpot; (void)maxSize;
  }

  void load(const LoadImage &image, LoadVec hotSpot, LoadVec size2d)
  {
    (void)image; (void)hotSpot; (void)size2d;
  }

  void load(LoadImage &&image, LoadVec hotSpot, float maxSize)
  {
    (void)image; (void)hotSpot; (void)maxSize;
  }

  void load(LoadImage &&image, LoadVec hotSpot, LoadVec size2d)
  {
    (void)image; (void)hotSpot; (void)size2d;
  }
};

static void choose_load_overload()
{
  LoadTarget target;
  const float maxSize = 1.0f;
  target.load(LoadImage(1 + 2), LoadVec(0.5f), maxSize);
}

int main()
{
  return 0;
}

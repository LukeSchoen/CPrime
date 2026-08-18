struct RvalueImage
{
  RvalueImage() {}
};

struct RvalueLoader
{
  int selected;

  RvalueLoader() : selected(0) {}

  void load(const RvalueImage &image, int value)
  {
    (void)image;
    selected = value;
  }

  void load(RvalueImage &&image, int value)
  {
    (void)image;
    selected = value + 1;
  }
};

static void choose_rvalue_overload()
{
  RvalueLoader loader;
  loader.load(RvalueImage(), 4);
  (void)loader;
}

int main()
{
  return 0;
}

struct StringLike
{
  StringLike() {}
  StringLike(const char *) {}
  StringLike operator+(const char *) const { return StringLike(); }
};

struct PathLike
{
  PathLike(const StringLike &) {}
};

struct ArrayLike
{
};

struct ImageLike
{
  ImageLike(const ArrayLike &) {}
  ImageLike(const PathLike &) {}
};

static StringLike AssetsPath()
{
  return StringLike();
}

static void load_image_from_path_expression()
{
  ImageLike image(AssetsPath() + "/Games/RC/Car.png");
  (void)image;
}

int main()
{
  return 0;
}

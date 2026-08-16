class Text
{
public:
  Text() { value = 0; }
  Text(const Text &rhs) { value = rhs.value; }
  Text &operator=(const Text &rhs)
  {
    value = rhs.value;
    return *this;
  }
  ~Text() {}

  int value;
};

struct Color
{
  float x;
  float y;
  float z;
  float w;
};

static Color one()
{
  Color c;
  c.x = 1;
  c.y = 1;
  c.z = 1;
  c.w = 1;
  return c;
}

struct Material
{
  Text name;
  double alpha;
  Color diffuse;
  Text path;
};

static Material makeMaterial()
{
  Material material;
  material.alpha = 1;
  material.diffuse = one();
  return material;
}

int main()
{
  Material material = makeMaterial();
  return material.alpha == 1 ? 0 : 1;
}

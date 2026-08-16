class Text
{
public:
  Text() {}
  Text(const Text &rhs) {}
  Text &operator=(const Text &rhs) { return *this; }
  ~Text() {}
};

template <typename T> class List
{
public:
  List() { data = 0; }
  ~List() {}
  void PushBack(T value) {}

private:
  T *data;
};

struct Vec3
{
  float x, y, z;
  Vec3() { x = y = z = 0; }
};

struct Vec2
{
  float x, y;
  Vec2() { x = y = 0; }
};

struct Vec4
{
  float x, y, z, w;
  Vec4() { x = y = z = w = 0; }
};

struct Vertex
{
  long long position;
  long long normal;
  long long color;
  long long texCoords;
};

struct Triangle
{
  Vertex v0;
  Vertex v1;
  Vertex v2;
  long long material;
};

struct Material
{
  Text name;
  double alpha;
  Vec4 diffuseColor;
  Vec4 ambientColor;
  Vec4 specularColor;
  Text diffuseMapPath;
};

static Vec4 one()
{
  Vec4 v;
  v.x = 1;
  v.y = 1;
  v.z = 1;
  v.w = 1;
  return v;
}

static Vec4 zero()
{
  Vec4 v;
  v.x = 0;
  v.y = 0;
  v.z = 0;
  v.w = 0;
  return v;
}

static Material makeMaterial()
{
  Material material;
  material.alpha = 1;
  material.diffuseColor = one();
  material.ambientColor = zero();
  material.specularColor = zero();
  return material;
}

class Mesh
{
public:
  Text sourceFileName;
  Text resourceDirectory;
  List<Vec3> positions;
  List<Vec4> colors;
  List<Vec3> normals;
  List<Vec2> texCoords;
  List<Triangle> triangles;
  List<Material> materials;
};

#include <windows.h>

int main()
{
  return 0;
}

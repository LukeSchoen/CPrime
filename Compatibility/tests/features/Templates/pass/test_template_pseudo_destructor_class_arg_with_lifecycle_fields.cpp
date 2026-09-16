// EXPECT_EXIT: 0

template<typename T>
class List
{
public:
  T *data;
  int size;

  List()
  {
    this->data = (T *)0;
    this->size = 0;
  }

  ~List()
  {
    this->ClearStorage();
  }

  void ClearStorage()
  {
    for (int i = 0; i < this->size; ++i)
      this->data[i].~T();
  }
};

class Material
{
public:
  List<int> names;
};

class Mesh
{
public:
  List<Material> materials;
};

int main(void)
{
  Mesh mesh;
  mesh.materials.size = 0;
  return mesh.materials.size;
}

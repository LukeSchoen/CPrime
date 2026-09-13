struct Mesh { int value; };

struct Copy {
  int value;
  Copy(Mesh &mesh) : value(mesh.value) {}
};

struct Holder {
  Mesh mesh;
  Holder() : mesh{9} {}
  int copy_value() { Copy copy(Holder::mesh); return copy.value; }
};

int main()
{
  Holder holder;
  return holder.copy_value() != 9;
}

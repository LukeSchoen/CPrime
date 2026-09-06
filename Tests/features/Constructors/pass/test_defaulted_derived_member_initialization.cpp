static int constructions, destructions;
struct Owner {
  int *data;
  Owner() : data(nullptr) { ++constructions; }
  ~Owner() { ++destructions; }
};
template<class T> struct Array {
  T storage;
  int size = 7;
  Array() = default;
};
struct Image : Array<Owner> {
  Image() = default;
};
struct VoidImage : Array<Owner> {
  VoidImage(void) = default;
};
int main() {
  {
    Image image;
    if (constructions != 1 || image.storage.data || image.size != 7) return 1;
    Image values[2];
    if (constructions != 3 || values[0].storage.data || values[1].size != 7) return 2;
    VoidImage with_void;
    if (constructions != 4 || with_void.storage.data || with_void.size != 7) return 3;
  }
  return destructions != 4;
}

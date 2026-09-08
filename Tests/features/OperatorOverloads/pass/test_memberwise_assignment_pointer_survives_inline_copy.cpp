// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -O2

// Memberwise assignment: inline copies of the vector
// members must not recycle the spill containing the enclosing object pointer.
struct Value {
  int value;
  Value(int n = 0) : value(n) {}
  Value &operator=(const Value &other) {
    value = other.value;
    return *this;
  }
};
struct Vector { double x, y, z, w; };
struct Material {
  Value first;
  double alpha;
  Vector diffuse, ambient, specular;
  Value last;
};

// Extra elements expose writes through an incorrectly advanced base pointer
// without allowing the original bug to write outside the allocation.
Material materials[4];
Material &destination() { return materials[0]; }
Material &source() { return materials[1]; }

int main() {
  Material original;
  original.first.value = 11;
  original.last.value = 77;
  original.alpha = 2;
  original.diffuse = {1, 2, 3, 4};
  original.ambient = {5, 6, 7, 8};
  original.specular = {9, 10, 11, 12};
  destination() = original;
  if (materials[0].first.value != 11 || materials[0].last.value != 77) return 1;
  if (materials[0].alpha != 2 || materials[0].diffuse.w != 4
      || materials[0].ambient.w != 8 || materials[0].specular.w != 12) return 2;
  source() = original;
  destination() = source();
  if (materials[0].last.value != 77 || materials[0].specular.x != 9) return 3;
  if (materials[2].first.value || materials[2].last.value
      || materials[3].first.value || materials[3].last.value) return 4;
  return 0;
}

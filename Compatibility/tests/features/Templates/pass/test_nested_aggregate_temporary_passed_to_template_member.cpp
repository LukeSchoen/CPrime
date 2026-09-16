extern "C" void *malloc(unsigned long long size);

template<typename T, typename U>
void ConstructAt(T *target, U &&input)
{
  new(target) T(input);
}

template<typename T>
struct Slot
{
  T *value;
  T snapshot;

  Slot()
  {
    value = (T *)malloc(sizeof(T));
  }

  void PushBack(T &&input)
  {
    snapshot = input;
    ConstructAt(value, input);
  }
};

template<typename T>
struct Vector3
{
  T x;
  T y;
  T z;

  Vector3() : x(0), y(0), z(0) {}
  Vector3(const T &newX, const T &newY, const T &newZ);
};

template<typename T>
Vector3<T>::Vector3(const T &newX, const T &newY, const T &newZ)
  : x(newX), y(newY), z(newZ)
{}

typedef Vector3<float> Vec3;

struct Cipher
{
  struct Atom
  {
    Vec3 position;
    Vec3 velocity;
  };

  Slot<Atom> atoms;

  void Initialize(float index)
  {
    atoms.PushBack(Atom{
      Vec3{index, index * 0.1f, index * 0.01f},
      Vec3{-index, -index * 2.0f, -index * 3.0f}
    });
  }
};

int main()
{
  Cipher cipher;
  if (!cipher.atoms.value)
    return 3;
  cipher.Initialize(2.0f);

  if (cipher.atoms.snapshot.position.x != 2.0f
      || cipher.atoms.snapshot.position.y != 0.2f
      || cipher.atoms.snapshot.position.z != 0.02f
      || cipher.atoms.snapshot.velocity.x != -2.0f
      || cipher.atoms.snapshot.velocity.y != -4.0f
      || cipher.atoms.snapshot.velocity.z != -6.0f)
    return 2;

  return cipher.atoms.value->position.x == 2.0f
      && cipher.atoms.value->position.y == 0.2f
      && cipher.atoms.value->position.z == 0.02f
      && cipher.atoms.value->velocity.x == -2.0f
      && cipher.atoms.value->velocity.y == -4.0f
      && cipher.atoms.value->velocity.z == -6.0f
    ? 0 : 1;
}

// EXPECT_EXIT: 0

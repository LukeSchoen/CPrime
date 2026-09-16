typedef unsigned char ui8;
typedef long long i64;

template<typename T>
class MiniList
{
public:
  MiniList() { count = 1; }
  i64 Size() const { return count; }
  T &operator[](i64 index) { return item; }
  const T &operator[](i64 index) const { return item; }

  T item;
  i64 count;
};

struct Vec3
{
  float x;
  float y;
  float z;
};

class EntropyLike
{
  struct Atom
  {
    Vec3 pos;
    Vec3 vel;
  };

public:
  ui8 ExtractEntropy();
  void ExtractEntropy(void *pData, i64 dataSize, ui8 *pResult);
  ui8 Read();
  MiniList<EntropyLike::Atom> atoms;
};

void EntropyLike::ExtractEntropy(void *pData, i64 dataSize, ui8 *pResult)
{
  *pResult = (ui8)dataSize;
}

ui8 EntropyLike::ExtractEntropy()
{
  return 1;
}

ui8 EntropyLike::Read()
{
  ui8 value = 0;
  for (i64 atomIndex = 0; atomIndex < atoms.Size(); ++atomIndex)
  {
    const Atom &atom = atoms[atomIndex];
    ExtractEntropy((void *)&atom.pos, (i64)sizeof(atom.pos), &value);
    ExtractEntropy((void *)&atom.vel, (i64)sizeof(atom.vel), &value);
  }
  return value;
}

int main()
{
  return 0;
}

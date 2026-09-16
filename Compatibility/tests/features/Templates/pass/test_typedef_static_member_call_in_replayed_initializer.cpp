template<typename T>
class Vector2
{
public:
  T x;
  T y;

  static Vector2 Zero()
  {
    Vector2 value;
    value.x = 0;
    value.y = 0;
    return value;
  }
};

typedef Vector2<int> Vector2I;

template<typename T>
class Array2
{
public:
  Array2(const Vector2I &size)
    : m_size(size)
  {
  }

  int sum() const { return m_size.x + m_size.y; }

private:
  Vector2I m_size = Vector2I::Zero();
};

int main()
{
  Array2<float> values(Vector2I::Zero());
  return values.sum() != 0;
}

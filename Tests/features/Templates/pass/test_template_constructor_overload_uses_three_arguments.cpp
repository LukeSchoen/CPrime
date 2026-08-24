namespace std
{
  template <typename T> class initializer_list
  {
    const T *first;
    unsigned long long count;
  public:
    const T *begin() const { return first; }
    const T *end() const { return first + count; }
    unsigned long long size() const { return count; }
  };
}

template <typename T> T replayZero() { return T(0); }

template <typename T>
struct ReplayVector3
{
  T x, y, z;

  ReplayVector3() = default;
  ReplayVector3(const T &value);
  ReplayVector3(const T &xValue, const T &yValue,
                const T &zValue = replayZero<T>());

  template <typename U>
  explicit ReplayVector3(const ReplayVector3<U> &other)
    : x(T(other.x)), y(T(other.y)), z(T(other.z))
  {
  }

  template <typename U, typename V, typename W>
  explicit ReplayVector3(const U &xValue, const V &yValue, const W &zValue)
    : x(T(xValue)), y(T(yValue)), z(T(zValue))
  {
  }
};

template <typename T>
struct ReplayList
{
  T values[4];
  int count;

  ReplayList();
  ReplayList(const std::initializer_list<T> &source);
  void PushBack(const std::initializer_list<T> &source);
};

template <typename T>
ReplayList<T>::ReplayList() : count(0)
{
}

template <typename T>
ReplayList<T>::ReplayList(const std::initializer_list<T> &source)
  : ReplayList()
{
  PushBack(source);
}

template <typename T>
void ReplayList<T>::PushBack(const std::initializer_list<T> &source)
{
  for (const T &value : source)
    values[count++] = value;
}

template <typename T>
ReplayVector3<T>::ReplayVector3(const T &value)
  : ReplayVector3(value, value, value)
{
}

template <typename T>
ReplayVector3<T>::ReplayVector3(const T &xValue, const T &yValue,
                               const T &zValue)
  : x(xValue), y(yValue), z(zValue)
{
}

typedef ReplayVector3<float> ReplayVec3;

int main()
{
  ReplayVector3<float> a(64.0f, 0.0f, 0.0f);
  ReplayVector3<float> b(64.0f, 48.0f, 0.0f);
  ReplayVector3<float> values[] = {
    ReplayVector3<float>(0.0f, 0.0f, 0.0f),
    ReplayVector3<float>(64.0f, 0.0f, 0.0f),
    ReplayVector3<float>(64.0f, 48.0f, 0.0f),
    ReplayVector3<float>(0.0f, 48.0f, 0.0f)
  };
  ReplayList<ReplayVec3> list = {
    ReplayVec3(0.0f, 0.0f, 0.0f),
    ReplayVec3(64.0f, 0.0f, 0.0f),
    ReplayVec3(64.0f, 48.0f, 0.0f),
    ReplayVec3(0.0f, 48.0f, 0.0f)
  };
  return a.x == 64.0f && a.y == 0.0f && a.z == 0.0f
      && b.x == 64.0f && b.y == 48.0f && b.z == 0.0f
      && values[0].x == 0.0f && values[0].y == 0.0f && values[0].z == 0.0f
      && values[1].x == 64.0f && values[1].y == 0.0f && values[1].z == 0.0f
      && values[2].x == 64.0f && values[2].y == 48.0f && values[2].z == 0.0f
      && values[3].x == 0.0f && values[3].y == 48.0f && values[3].z == 0.0f
      && list.values[0].x == 0.0f && list.values[0].y == 0.0f && list.values[0].z == 0.0f
      && list.values[1].x == 64.0f && list.values[1].y == 0.0f && list.values[1].z == 0.0f
      && list.values[2].x == 64.0f && list.values[2].y == 48.0f && list.values[2].z == 0.0f
      && list.values[3].x == 0.0f && list.values[3].y == 48.0f && list.values[3].z == 0.0f
      ? 0 : 1;
}

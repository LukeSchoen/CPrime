// EXPECT_EXIT: 0

template<typename T>
struct Box
{
  T x, y, z, w;

  template<typename U> static Box<U> Build(U a, U b, U c, U d)
  {
    Box<U> r;
    r.x = a;
    r.y = b;
    r.z = c;
    r.w = d;
    return r;
  }

  template<typename U> auto Translated(U t) const;

  static Box<T> Translation(T t)
  {
    Box<T> b;
    b.x = t;
    b.y = b.z = b.w = (T)0;
    return b.Translated(t);
  }
};

template<typename T>
template<typename U>
auto Box<T>::Translated(U t) const
{
  return Build(x + t, y, z, w);
}

int main()
{
  Box<float> b = Box<float>::Translation(2.0f);
  return b.x == 4.0f ? 0 : 1;
}

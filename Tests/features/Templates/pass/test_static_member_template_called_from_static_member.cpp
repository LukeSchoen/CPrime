// EXPECT_EXIT: 0

template<typename T>
struct Box
{
  T v;

  template<typename U> static Box<U> Make(U a)
  {
    Box<U> b;
    b.v = a;
    return b;
  }

  static Box<T> One() { return Make((T)1); }
};

int main()
{
  Box<float> b = Box<float>::One();
  return b.v == 1.0f ? 0 : 1;
}

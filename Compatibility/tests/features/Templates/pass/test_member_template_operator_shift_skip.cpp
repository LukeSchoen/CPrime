template <typename T>
struct Vec
{
  template <typename U> auto operator <<(const Vec<U> &o) const;
  template <typename U> Vec<T> &operator -=(const Vec<U> &o);
};

int main()
{
  return 0;
}

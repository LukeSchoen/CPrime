// EXPECT_EXIT: 0
template<typename T>
struct Holder
{
  T value;
  T get();
};

template<typename T>
T Holder<T>::get()
{
  return value;
}

int main(void)
{
  Holder<int> a;
  Holder<Holder<int> > b;
  Holder<Holder<Holder<int> > > c;

  a.value = 3;
  b.value = a;
  c.value = b;

  return c.get().get().get() == 3 ? 0 : 1;
}


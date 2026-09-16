template<typename T>
struct ShortBox
{
  T value;
};

template<typename T>
struct NestedShortBox
{
  ShortBox<T> box;

  T Read() const
  {
    return box.value;
  }
};

template<typename T>
int short_value(T value)
{
  return (int)value;
}

int main()
{
  ShortBox<short> signed_box;
  ShortBox<unsigned short> unsigned_box;
  NestedShortBox<short> nested;

  signed_box.value = (short)-7;
  unsigned_box.value = (unsigned short)60000;
  nested.box = signed_box;

  if (nested.Read() != (short)-7)
    return 1;
  if (short_value<short>(signed_box.value) != -7)
    return 2;
  if (short_value<unsigned short>(unsigned_box.value) != 60000)
    return 3;
  return 0;
}

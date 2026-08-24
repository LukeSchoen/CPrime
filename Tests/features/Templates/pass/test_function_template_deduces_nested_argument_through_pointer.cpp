template<typename T>
class Box
{
};

template<typename T>
int ElementSize(Box<T> *value)
{
  (void)value;
  return sizeof(T);
}

int main()
{
  Box<int> value;
  return ElementSize(&value) == sizeof(int) ? 0 : 1;
}

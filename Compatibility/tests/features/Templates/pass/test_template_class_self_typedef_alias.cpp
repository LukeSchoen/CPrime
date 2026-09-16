template <typename T>
struct Box
{
  typedef Box<T> BoxType;
};

typedef Box<int> IntBox;

int main()
{
  IntBox b;
  (void)b;
  return 0;
}

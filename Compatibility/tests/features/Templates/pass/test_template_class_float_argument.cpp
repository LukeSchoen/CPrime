template <typename T>
struct FloatBox
{
  T value;
};

void take_box(const FloatBox<float> &box)
{
  (void)box;
}

int main()
{
  return 0;
}

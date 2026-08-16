// EXPECT_EXIT: 0

namespace ns_param
{
  template<class T>
  class Box
  {
  public:
    T value;
  };
}

int read_box(const ns_param::Box<int> &box)
{
  return box.value;
}

int main(void)
{
  ns_param::Box<int> box;
  box.value = 9;
  return read_box(box) - 9;
}

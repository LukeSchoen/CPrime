// EXPECT_EXIT: 0

template<typename T>
class Box
{
public:
  Box()
  {
    value = 3;
  }

  int value;
};

template<typename T>
class Box;

typedef Box<int> IntBox;

class Holder
{
public:
  IntBox box;
};

int main()
{
  Holder holder;
  return holder.box.value == 3 ? 0 : 1;
}

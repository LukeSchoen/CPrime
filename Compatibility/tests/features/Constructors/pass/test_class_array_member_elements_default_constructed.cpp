// EXPECT_EXIT: 0

int constructed;
int assigned_ok;

class SmallString
{
public:
  int ready;
  int value;

  SmallString()
  {
    ready = 1;
    value = 0;
    constructed += 1;
  }

  SmallString& operator=(const SmallString& other)
  {
    if (ready == 1)
      assigned_ok += 1;
    value = other.value;
    return *this;
  }
};

class Holder
{
public:
  SmallString items[4];
};

int main(void)
{
  Holder holder;
  SmallString source;
  source.value = 9;
  holder.items[2] = source;
  if (constructed != 5)
    return 1;
  if (assigned_ok != 1)
    return 2;
  return holder.items[2].value == 9 ? 0 : 3;
}

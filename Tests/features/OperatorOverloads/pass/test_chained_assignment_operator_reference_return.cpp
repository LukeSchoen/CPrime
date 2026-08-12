// EXPECT_EXIT: 0

class Box
{
public:
  Box();
  Box(int value);
  Box& operator=(const Box &rhs);

public:
  int value;
  int assigned;
};

Box::Box()
{
  this->value = 0;
  this->assigned = 0;
}

Box::Box(int value)
{
  this->value = value;
  this->assigned = 0;
}

Box& Box::operator=(const Box &rhs)
{
  this->value = rhs.value;
  this->assigned = this->assigned + 1;
  return *this;
}

int main(void)
{
  Box a;
  Box b;
  Box c(9);

  b = c;
  if (b.value != 9)
    return 1;
  if (b.assigned != 1)
    return 2;

  a = b = c;
  if (b.value != 9)
    return 3;
  if (b.assigned != 2)
    return 4;
  if (a.value != 9)
    return 5;
  if (a.assigned != 1)
    return 6;

  return 0;
}

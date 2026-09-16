// EXPECT_EXIT: 0
// EXPECT_STDOUT:
class Box
{
public:
  Box();
  Box(int value);
  Box operator=(Box other);

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

Box Box::operator=(Box other)
{
  this->value = other.value + 10;
  this->assigned = this->assigned + 1;
  return *this;
}

int main(void)
{
  Box a(7);
  Box b;
  Box c;

  b = a;
  c = b;

  if (b.value != 17)
    return 1;
  if (b.assigned != 1)
    return 2;
  if (c.value != 27)
    return 3;
  if (c.assigned != 1)
    return 4;
  return 0;
}

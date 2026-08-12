// EXPECT_EXIT: 0
struct Vec
{
  int value;

  Vec& operator=(int rhs);
  Vec& operator-=(int rhs);
  Vec& operator=(const Vec& rhs);
  int operator[](int idx) const;
  int operator!();
};

Vec& Vec::operator=(int rhs)
{
  this->value = rhs;
  return *this;
}

Vec& Vec::operator-=(int rhs)
{
  this->value -= rhs;
  return *this;
}

Vec& Vec::operator=(const Vec& rhs)
{
  this->value = rhs.value;
  return *this;
}

int Vec::operator[](int idx) const
{
  return this->value + idx + 100;
}

int Vec::operator!()
{
  return this->value == 0;
}

int main(void)
{
  Vec a = {0};
  Vec b = {7};
  const Vec c = {8};
  int ok = 1;

  a = 5;
  if (a.value != 5) ok = 0;
  a -= 2;
  if (a.value != 3) ok = 0;
  a = b;
  if (a.value != 7) ok = 0;
  if (c[4] != 112) ok = 0;
  if ((!a) != 0) ok = 0;

  return ok ? 0 : 1;
}


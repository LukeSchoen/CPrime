// EXPECT_EXIT: 0
struct Camera
{
  int value;

  void init(int v);
  int get();
};

void Camera::init(int v)
{
  this->value = v;
}

int Camera::get()
{
  return this->value;
}

int main(void)
{
  struct Camera c;
  c.init(7);
  return c.get() == 7 ? 0 : 1;
}

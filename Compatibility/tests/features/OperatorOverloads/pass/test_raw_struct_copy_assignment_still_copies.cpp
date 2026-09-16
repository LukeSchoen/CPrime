// EXPECT_EXIT: 0
// EXPECT_STDOUT:
class Pair
{
public:
  Pair();
  Pair(int left, int right);

public:
  int left;
  int right;
};

Pair::Pair()
{
  this->left = 0;
  this->right = 0;
}

Pair::Pair(int left, int right)
{
  this->left = left;
  this->right = right;
}

static Pair make_pair(void)
{
  Pair p(5, 9);
  return p;
}

int main(void)
{
  Pair a(1, 2);
  Pair b;
  Pair c;

  b = a;
  if (b.left != 1)
    return 1;
  if (b.right != 2)
    return 2;

  c = make_pair();
  if (c.left != 5)
    return 3;
  if (c.right != 9)
    return 4;
  return 0;
}

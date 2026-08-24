template<class T>
void Exchange(T &left, T &right)
{
  T value = left;
  left = right;
  right = value;
}

int main()
{
  int a = 3;
  int b = 8;
  int *left = &a;
  int *right = &b;
  Exchange(left, right);
  return left == &b && right == &a ? 0 : 1;
}

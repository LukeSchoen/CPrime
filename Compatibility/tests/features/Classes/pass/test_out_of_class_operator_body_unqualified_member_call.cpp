class AssignLike
{
public:
  AssignLike &operator=(const AssignLike &rhs);

protected:
  void Close() {}
  int value;
};

AssignLike &AssignLike::operator=(const AssignLike &rhs)
{
  Close();
  value = rhs.value;
  return *this;
}

int main()
{
  AssignLike a;
  AssignLike b;
  a = b;
  return 0;
}

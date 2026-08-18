int child_constructed;

struct Child
{
  Child() { child_constructed = 1; }
};

struct Aggregate
{
  Child child;
};

int main()
{
  Aggregate value = Aggregate();
  (void)value;
  return child_constructed ? 0 : 1;
}

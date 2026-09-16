struct A
{
  static int foo() asm("_custom_member_routine");
};

int A::foo()
{
  return 42;
}

int main()
{
  return A::foo() != 42;
}

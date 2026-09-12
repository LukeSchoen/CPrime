namespace ns
{
template <class T>
struct A
{
  typedef int type;
};
}

ns::A<int>::type value;

int main()
{
  return value;
}

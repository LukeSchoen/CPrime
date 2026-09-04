namespace compatibility
{
int value = 27;

int Read()
{
  return value;
}
}

using namespace compatibility;

int main()
{
  return value == 27 && Read() == 27 ? 0 : 1;
}

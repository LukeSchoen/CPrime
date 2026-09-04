int select_value()
{
  return 3;
}

namespace shadow
{
int select_value()
{
  return ::select_value() + 4;
}
}

using namespace shadow;

int main()
{
  return shadow::select_value() == 7 ? 0 : 1;
}

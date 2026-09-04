namespace library
{
class Base
{
public:
  Base() : value(53) {}
  int value;
};
}

using namespace library;

int main()
{
  struct Local : Base
  {
    Local() {}
  };

  Local local;
  return local.value == 53 ? 0 : 1;
}

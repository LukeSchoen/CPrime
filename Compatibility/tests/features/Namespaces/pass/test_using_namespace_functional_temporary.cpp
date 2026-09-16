namespace compatibility
{
class Value
{
public:
  explicit Value(int initial) : data(initial) {}

  int Get() const
  {
    return data;
  }

private:
  int data;
};
}

using namespace compatibility;

int main()
{
  return Value(31).Get() == 31 ? 0 : 1;
}

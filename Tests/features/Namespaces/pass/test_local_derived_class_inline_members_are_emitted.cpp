namespace compatibility
{
class Visitor
{
public:
  virtual ~Visitor() {}
};
}

using namespace compatibility;

int main()
{
  struct TestUtil : Visitor
  {
    TestUtil() : value(0) {}

    void Set(int next)
    {
      value = next;
    }

    int Get() const
    {
      return value;
    }

  private:
    int value;
  };

  TestUtil util;
  util.Set(61);
  return util.Get() == 61 ? 0 : 1;
}

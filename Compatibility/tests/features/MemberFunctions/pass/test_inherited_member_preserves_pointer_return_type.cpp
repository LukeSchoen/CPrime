class Base
{
public:
  const char* Value() const;
};

const char* Base::Value() const
{
  return "base";
}

class Derived : public Base
{
};

int main()
{
  Derived derived;
  const char* value = derived.Value();
  return value && value[0] == 'b' ? 0 : 1;
}

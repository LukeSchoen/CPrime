class ConditionalValue
{
public:
  int value;
  long long padding1;
  long long padding2;

  ConditionalValue(int v = 0) : value(v), padding1(0x11223344), padding2(0x55667788) {}
  ConditionalValue(const ConditionalValue &other)
      : value(other.value), padding1(other.padding1), padding2(other.padding2) {}
  ConditionalValue &operator=(const ConditionalValue &other)
  {
    value = other.value;
    padding1 = other.padding1;
    padding2 = other.padding2;
    return *this;
  }
  bool Empty() const { return value == 0; }
};

ConditionalValue operator+(const ConditionalValue &left,
                           const ConditionalValue &right)
{
  return ConditionalValue(left.value + right.value);
}

static ConditionalValue selectedValue;
static ConditionalValue baseValue;

ConditionalValue SelectValue()
{
  return !selectedValue.Empty()
      ? selectedValue
      : baseValue + ConditionalValue(3);
}

int ScrambleReturnRegister() { return 0; }

int main()
{
  baseValue = ConditionalValue(20);
  ScrambleReturnRegister();
  ConditionalValue first = SelectValue();
  if (first.value != 23)
    return 1;

  selectedValue = ConditionalValue(41);
  ScrambleReturnRegister();
  ConditionalValue second = SelectValue();
  return second.value == 41 ? 0 : 2;
}

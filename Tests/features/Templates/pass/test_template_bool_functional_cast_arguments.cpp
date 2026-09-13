template <bool Value> struct Flag {
  bool value;
  Flag() : value(Value) {}
};

Flag<bool(1)> first;
Flag<bool(2>1)> second;
Flag<(bool)(3>2)> third;

int main()
{
  return !first.value || !second.value || !third.value;
}

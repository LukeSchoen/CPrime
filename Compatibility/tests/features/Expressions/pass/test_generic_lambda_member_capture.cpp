struct Counter
{
  int base;

  int ownBase() { return base; }

  int run()
  {
    auto explicitField = [this](auto value) { return base + value; };
    auto implicitField = [&](auto value) { return base + value; };
    auto explicitArrow = [this](auto value) { return this->base + value; };
    auto memberCall = [this](auto value) { return ownBase() + value; };
    auto parameterShadow = [this](auto base) { return base + 1; };
    auto localShadow = [this](auto value) {
      int base = 5;
      return base + value;
    };
    auto innerShadow = [this](auto value) {
      { int base = 99; (void)base; }
      return base + value;
    };

    if (explicitField(35) != 42) return 1;
    if (implicitField(35) != 42) return 2;
    if (explicitArrow(35) != 42) return 3;
    if (memberCall(35) != 42) return 4;
    if (parameterShadow(41) != 42) return 5;
    if (localShadow(37) != 42) return 6;
    return innerShadow(35) != 42;
  }
};

template<class T>
struct Box
{
  T value;

  auto make()
  {
    return [this](auto increment) { return value + increment; };
  }
};

int main()
{
  Counter counter{7};
  if (counter.run() != 0) return counter.run();
  Box<int> box{7};
  auto add = box.make();
  return add(35) != 42;
}

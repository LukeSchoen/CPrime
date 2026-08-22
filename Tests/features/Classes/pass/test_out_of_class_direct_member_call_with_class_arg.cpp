template<typename T>
class DirectMemberArg
{
public:
  DirectMemberArg() : x(0) {}
  DirectMemberArg(T value) : x(value) {}

  T x;
};

typedef DirectMemberArg<int> DirectMemberArgI;

DirectMemberArgI MakeDirectMemberArg(int value)
{
  return DirectMemberArgI(value);
}

class DirectMemberCaller
{
public:
  void SetValue(const DirectMemberArgI &value);
  void Apply(int value);
};

void DirectMemberCaller::Apply(int value)
{
  SetValue(MakeDirectMemberArg(value));
}

void DirectMemberCaller::SetValue(const DirectMemberArgI &value)
{
  (void)value;
}

int main()
{
  return 0;
}

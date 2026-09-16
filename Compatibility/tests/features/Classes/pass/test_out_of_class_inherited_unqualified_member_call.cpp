class ReplayBase
{
public:
  ReplayBase() : value(1) {}
  int GetValue() const;
  void SetValue(int next);

  int value;
};

class ReplayDerived : public ReplayBase
{
public:
  void Apply(int delta);
};

int ReplayBase::GetValue() const { return value; }
void ReplayBase::SetValue(int next) { value = next; }

void ReplayDerived::Apply(int delta)
{
  SetValue(GetValue() + delta);
}

int main()
{
  return 0;
}

template<typename T>
class ChainAutoVec
{
public:
  ChainAutoVec() : x(0) {}
  ChainAutoVec(T value) : x(value) {}

  template<typename U> auto operator+(const ChainAutoVec<U> &rhs) const;

  T x;
};

template<typename T>
ChainAutoVec<T> MakeChainAutoVec(T value)
{
  return ChainAutoVec<T>(value);
}

template<typename T>
template<typename U>
auto ChainAutoVec<T>::operator+(const ChainAutoVec<U> &rhs) const
{
  return MakeChainAutoVec(x + rhs.x);
}

class ChainBase
{
public:
  ChainBase() : value(1) {}
  ChainAutoVec<int> GetValue() const;
  void SetValue(const ChainAutoVec<int> &next);

  ChainAutoVec<int> value;
};

class ChainDerived : public ChainBase
{
public:
  void Apply();
};

ChainAutoVec<int> ChainBase::GetValue() const { return value; }
void ChainBase::SetValue(const ChainAutoVec<int> &next) { (void)next; }

void ChainDerived::Apply()
{
  ChainAutoVec<int> a(2);
  ChainAutoVec<int> b(3);
  SetValue(GetValue() + a + b);
}

int main()
{
  return 0;
}

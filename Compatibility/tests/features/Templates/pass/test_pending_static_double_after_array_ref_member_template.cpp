typedef long long i64;

template<typename T> class ArrayMemberOwner
{
public:
  template<i64 N> void Add(const T (&values)[N]);
  void Add(const T *values, i64 count);
};

template<typename T> template<i64 N>
void ArrayMemberOwner<T>::Add(const T (&values)[N])
{
  Add(values, N);
}

typedef ArrayMemberOwner<int> IntArrayMemberOwner;

template<typename T> struct StaticDoubleHelper;

template<> struct StaticDoubleHelper<double>
{
  static double Value() { return 2.718281828459045; }
};

int main()
{
  return StaticDoubleHelper<double>::Value() != 2.718281828459045;
}

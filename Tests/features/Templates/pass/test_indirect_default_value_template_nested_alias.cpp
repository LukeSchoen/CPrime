template<long long A, long long B>
struct GreatestCommonDivisor
{
  static constexpr long long value = GreatestCommonDivisor<B, A % B>::value;
};

template<long long A>
struct GreatestCommonDivisor<A, 0>
{
  static constexpr long long value = A;
};

template<long long Numerator, long long Denominator = 1>
struct Ratio
{
  static constexpr long long divisor =
    GreatestCommonDivisor<Numerator, Denominator>::value;
  static constexpr long long numerator = Numerator / divisor;
  static constexpr long long denominator = Denominator / divisor;
  typedef Ratio<numerator, denominator> type;
};

namespace time_units
{
  template<class Period = Ratio<1> >
  struct Duration
  {
    typedef typename Period::type period;
  };

  typedef Duration<> Seconds;
}

int main()
{
  return time_units::Seconds::period::numerator == 1
      && time_units::Seconds::period::denominator == 1 ? 0 : 1;
}

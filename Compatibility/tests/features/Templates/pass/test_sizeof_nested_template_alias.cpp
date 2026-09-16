// EXPECT_EXIT: 0
template<class T> struct Wrap { typedef T Type; enum { answer = 9 };
  static const long long value = 11; };
template<int N> struct Value { typedef Wrap<char[N]> Alias; };
int main() { return sizeof(Value<7>::Alias::Type) != 7
    || sizeof(Value<3>::Alias::Type) != 3
    || sizeof(Value<3>::Alias::value) != sizeof(long long)
    || (Value<3>::Alias::answer) != 9 || (Value<3>::Alias::value) != 11; }

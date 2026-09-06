namespace N {
template<class T> struct Value { typedef T rep; };
typedef Value<int> Number;
struct Clock {
  typedef Number Value;
  typedef Value::rep rep;
};
}
typedef typename N::Number::rep Integer;
template<class T> int read() { typedef typename T::rep R; return sizeof(R); }
int main() { return sizeof(Integer)!=sizeof(int) || read<N::Number>()!=sizeof(int)
  || sizeof(N::Clock::rep)!=sizeof(int); }

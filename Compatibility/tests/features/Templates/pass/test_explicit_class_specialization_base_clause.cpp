// An explicit class template specialization may have a base clause. The
// nested-class specialization probe (`template<> class Owner::Member<A>`) read
// the single colon as the start of a `::` qualifier and rejected the
// specialization with "':' expected".

struct Plain
{
  Plain() : v(7) {}
  int v;
};

template<typename T> class Obs;

template<> class Obs<unsigned char> : public Plain {};

template<> class Obs<short> : public Plain
{
public:
  int extra() const { return v * 2; }
};

int main()
{
  Obs<unsigned char> a;
  Obs<short> b;
  if (a.v != 7) return 1;
  if (b.extra() != 14) return 2;
  return 0;
}

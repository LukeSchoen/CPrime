// EXPECT_EXIT: 0

// Explicit instantiation of a member conversion template selects the target
// from the conversion-type-id and keeps the out-of-class primary body.
namespace explicit_instantiation_case
{
struct S
{
  template<class T>
  operator T();
};

template<class T>
S::operator T()
{
  return T();
}

template S::operator int();
}

// Explicit specializations of declaration-only conversion templates supply
// the target's body.  When T and T* both deduce to char*, the more
// specialized T* pattern is the explicit specialization actually used.
namespace explicit_specialization_case
{
char c;

struct S
{
  template<class T>
  operator T*();

  template<class T>
  operator T();
};

template<>
S::operator int()
{
  return 2;
}

template<>
S::operator char*()
{
  return &c;
}
}

int main()
{
  {
    explicit_instantiation_case::S s;
    if (s.operator int() != 0)
      return 1;
  }
  {
    explicit_specialization_case::S s;
    if (s.operator int() != 2)
      return 2;
    if (static_cast<char *>(s) != &explicit_specialization_case::c)
      return 3;
  }
  return 0;
}

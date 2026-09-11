/* A friend declaration may name a specialization of a member conversion
   function template through its target type alone.  The specialization is
   deduced from the written conversion target; no explicit template argument
   list and no definition of the member template is required. */
struct A
{
  template <typename T> operator T ();
};

struct X {};
struct Y {};

struct B
{
  friend A::operator X ();
  friend A::operator Y ();
};

int main ()
{
  return 0;
}

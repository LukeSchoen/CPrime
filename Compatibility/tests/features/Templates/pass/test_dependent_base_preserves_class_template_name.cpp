/* A class template whose base clause names a dependent qualified type
   (`wrap<T>::type`) is still registered as a class template.  The qualified
   template-id belongs to the base clause and must not be mistaken for an
   out-of-class member class definition, which would leave the class template
   name unknown to later uses. */

template<class T> struct wrap { typedef T type; };

struct payload
{
  payload () : value_ (7) {}
  int value_;
};

template<class T> class derived : public wrap<T>::type
{
public:
  int probe () const { return this->value_; }
};

int main ()
{
  derived<payload> d;
  return d.probe () == 7 ? 0 : 1;
}

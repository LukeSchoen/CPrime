// EXPECT_EXIT: 23
// A member class template of a class template can be partially specialized
// for the enclosing template's parameter, and a use inside the class body
// must see that specialization while the enclosing class is instantiated.
template <class T>
class Owner {
private:
  template <T tag, T value>
  struct Slot {};

public:
  typedef typename Slot<1, 23>::Payload Type;
};

template <class T>
template <T value>
struct Owner<T>::Slot<1, value> {
  typedef int Payload;
};

int main()
{
  Owner<unsigned>::Type value = 23;
  return value;
}

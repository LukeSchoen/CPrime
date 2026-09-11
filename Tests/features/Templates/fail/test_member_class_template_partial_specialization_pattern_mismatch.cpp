// EXPECT_COMPILE_FAIL: 1
// The specialization pattern does not match the argument list, so the primary
// member template is selected and has no such member. Report a diagnostic
// instead of crashing while comparing value arguments.
template <class T>
class Owner {
private:
  template <T tag, T value>
  struct Slot {};

public:
  typedef typename Slot<0, 0>::Payload Type;
};

template <class T>
template <T value>
struct Owner<T>::Slot<1, value> {
  typedef void Payload;
};

template class Owner<unsigned>;

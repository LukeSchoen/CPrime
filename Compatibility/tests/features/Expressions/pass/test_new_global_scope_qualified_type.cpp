/* A new-expression whose type is spelled with an explicit global scope
   operator: `new ::T(args)`.

   The primary parser took the `::` that follows `new` as the qualifier of a
   namespace named `new`, so the expression became a call of an invented
   `new::T` function and the constructor arguments were never seen. */

struct Convertible
{
  int value;

  Convertible() : value(0) {}
  explicit Convertible(int argument) : value(argument) {}
};

namespace nested
{
  struct Convertible
  {
    int value;

    explicit Convertible(int argument) : value(argument) {}
  };
}

int main()
{
  Convertible *plain = new ::Convertible(7);
  if (plain->value != 7)
    return 1;
  delete plain;

  Convertible *empty = new ::Convertible();
  if (empty->value != 0)
    return 2;
  delete empty;

  nested::Convertible *qualified = new nested::Convertible(9);
  if (qualified->value != 9)
    return 3;
  delete qualified;

  return 0;
}

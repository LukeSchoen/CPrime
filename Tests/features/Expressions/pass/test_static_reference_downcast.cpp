struct Left { int left; };
struct Right { int right; };
struct Derived : Left, Right { int value; };
int main() {
  Derived object;
  object.value = 19;
  Right &base = object;
  const Right &constant = object;
  return &static_cast<Derived&>(base) != &object
      || static_cast<const Derived&>(constant).value != 19;
}

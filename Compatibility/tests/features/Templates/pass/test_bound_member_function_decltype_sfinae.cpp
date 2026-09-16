struct Object { int value; void function() {} };
template<class T, class M>
auto data(T &object, M member, int) -> decltype(object.*member)
{ return object.*member; }
template<class T, class M>
int data(T &, M, long) { return 7; }
int main() {
  Object object = {3};
  data(object, &Object::value, 0) = 5;
  return data(object, &Object::function, 0) != 7
      || data(object, &Object::value, 0) != 5;
}

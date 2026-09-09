// EXPECT_EXIT: 0
struct Value { int first; int second; };
template<class Owner, class Member> int owner_size(Member Owner::*member) {
  return sizeof(Owner);
}
template<class Owner, class Member>
Member read(Owner* object, Member Owner::*member) {
  Member Owner::*saved = member;
  return object->*saved;
}
int main() {
  Value value = {3, 7};
  return read(&value, &Value::first) != 3 || read(&value, &Value::second) != 7
      || owner_size(&Value::second) != sizeof(Value);
}

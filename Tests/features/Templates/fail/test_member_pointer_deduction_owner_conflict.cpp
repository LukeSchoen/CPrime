// EXPECT_COMPILE_FAIL: 1
struct First { int value; };
struct Second { int value; };
template<class Owner, class Member> Member read(Owner* object, Member Owner::*member) {
  return object->*member;
}
int main() { First object; return read(&object, &Second::value); }

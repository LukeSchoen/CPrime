// Explicit specializations of member class templates retain the member's
// declaration identity when the owner is an ordinary class, including a
// private class-head declaration.
class PrivateOwner
{
  template<class T> struct Member;
};

template<> struct PrivateOwner::Member<int> {};

class Owner
{
  template<class T> struct Value;
public:
};

template<> struct Owner::Value<int> { static const int value = 7; };

int main()
{
  PrivateOwner::Member<int> private_value;
  (void)private_value;
  return Owner::Value<int>::value == 7 ? 0 : 1;
}

// EXPECT_EXIT: 0
namespace Space {
  struct Value { int value; Value(int input) : value(input) {} };
  typedef Value Alias;
}
struct Object : virtual Space::Alias {
  Object() : ::Space::Alias(7) {}
};
struct Names { typedef Space::Value Parent; };
struct Other : Names::Parent {
  Other() : Names::Parent(11) {}
};
int main() {
  Object object;
  Other other;
  return object.value != 7 || other.value != 11;
}

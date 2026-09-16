struct Base { int value = 7; };
struct Left : virtual Base {};
struct Right : virtual Base {};
struct Diamond : Left, Right {};
struct Container : Base {};
struct First : virtual Container {};
struct Second : virtual Container {};
struct SharedContainer : First, Second {};
int main() {
  int Base::*member = &Base::value;
  Diamond diamond;
  SharedContainer shared;
  return (diamond.*member) != 7 || (shared.*member) != 7;
}

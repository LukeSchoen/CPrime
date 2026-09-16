struct Value {
  int number;
  int read() const { return number; }
  auto generic() { return [=, *this](auto amount) { return number + this->read() + amount; }; }
  auto nested() const { return [*this] { return [*this] { return this->read(); }; }; }
  auto factory() { return [*this](auto amount) { return [*this, amount] { return number + amount; }; }; }
};
int main() {
  Value value{4};
  auto generic = value.generic();
  auto outer = value.nested();
  auto factory = value.factory();
  value.number = 20;
  auto inner = outer();
  auto made = factory(6);
  return generic(3) != 11 || inner() != 4 || made() != 10;
}

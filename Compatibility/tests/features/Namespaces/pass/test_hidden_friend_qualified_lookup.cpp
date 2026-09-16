namespace API {
  struct Value { friend int inspect(Value) { return 7; } friend void item(); };
  namespace { int item = 3; }
}
int main() { API::Value value; return API::item != 3 || inspect(value) != 7; }

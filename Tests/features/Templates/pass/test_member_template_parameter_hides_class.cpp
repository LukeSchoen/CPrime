// EXPECT_EXIT: 0
struct Value {
  template<class Value> Value read(Value value) { Value copy = value; return sizeof(Value) + copy; }
  template<class T, class Value> Value second(T unused, Value value) { return value; }
};
int main() { Value value; return value.read(3) != sizeof(int) + 3
    || value.read((char)5) != sizeof(char) + 5 || value.second(1, 9L) != 9; }

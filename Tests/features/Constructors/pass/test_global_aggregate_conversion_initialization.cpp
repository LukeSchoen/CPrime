struct value {
  int number;
  operator int();
};

struct aggregate {
  value first, second;
  int converted;
};

value source;
aggregate object = {4, source, source};
value::operator int() { return 42; }

int main() {
  return object.first.number != 4 || object.second.number != 0
      || object.converted != 42;
}

// EXPECT_EXIT: 0

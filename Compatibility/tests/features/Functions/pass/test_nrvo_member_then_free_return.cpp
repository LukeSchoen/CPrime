struct Result { int value; };
struct Factory {
  Result make() { Result result{3}; return result; }
};
int member_result() { Factory factory; return factory.make().value; }
Result free_result() { Result result{7}; return result; }
int main() { return member_result() != 3 || free_result().value != 7; }

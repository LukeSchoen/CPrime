// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
template<class T> struct Base {
  struct Node { Node* child = nullptr; };
  using NodePtr = Node*;
  NodePtr root = nullptr;
  int value = 7;
};
struct Derived : Base<int> { Derived() {} };
int main() {
  Derived value;
  if (value.root != nullptr || value.value != 7) return 1;
  Base<int>* allocated = new Base<int>();
  int result = allocated->root != nullptr || allocated->value != 7;
  delete allocated;
  return result ? 2 : 0;
}

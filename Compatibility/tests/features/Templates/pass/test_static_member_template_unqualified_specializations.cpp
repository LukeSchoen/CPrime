struct Equal { bool operator()(int a, int b) const { return a == b; } };
struct Different { bool operator()(int a, int b) const { return a != b; } };
class Node {
  template<class Compare> static bool compare(Node* a, Node* b, const Compare& op) {
    return op(a->value, b->value);
  }
public:
  int value;
  bool eval(Node* other, bool equal) {
    if (equal) return compare(this, other, Equal());
    return compare(this, other, Different());
  }
};
int main() { Node a, b; a.value = 1; b.value = 2; return a.eval(&b, true) || !a.eval(&b, false); }

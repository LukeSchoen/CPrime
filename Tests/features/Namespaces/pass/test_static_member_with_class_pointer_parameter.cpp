namespace Library {
  struct Node {
    int n;
    static int read(Node *node);
    static int read(Node *node, int offset);
    int call();
  };
  int Node::call() { return read(this) + read(this, 2); }
  int Node::read(Node *node) { return node->n; }
  int Node::read(Node *node, int offset) { return node->n + offset; }
}
int main() {
  Library::Node node;
  node.n = 7;
  return node.call() != 16 || Library::Node::read(&node) != 7;
}

namespace markup {
enum node_type { empty, document, element };
struct node {
  node_type value;
  explicit node(node_type v) : value(v) {}
  node_type type() const;
  bool can_append() const;
};
node_type node::type() const { return value; }
bool node::can_append() const { return type() == document || type() == element; }
}

int main()
{
  markup::node doc(markup::document);
  markup::node child(markup::element);
  markup::node null(markup::empty);
  if (!doc.can_append() || !child.can_append() || null.can_append()) return 1;
  if (doc.type() != markup::document) return 2;
  if (markup::node_type() != markup::empty) return 3;
  return 0;
}

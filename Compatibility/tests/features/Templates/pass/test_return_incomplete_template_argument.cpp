template<class T> struct Range { T first, last; };
struct NodeIterator;
struct Node { Range<NodeIterator> children(); };
struct NodeIterator { int position; };
Range<NodeIterator> Node::children() { return {{3}, {7}}; }
int main() { Node n; return n.children().last.position != 7; }

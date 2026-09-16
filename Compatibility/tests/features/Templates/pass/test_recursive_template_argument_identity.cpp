// EXPECT_EXIT: 0
template<class T> struct Wrap {};
template<class T, class U = Wrap<T> > struct Node;
template<class T> struct Holder {};
template<class T, class U> struct Node : Holder<Node<U> > {
    friend int value(Node) { return 42; }
};
int main() { Node<int> n; return value(n) != 42; }

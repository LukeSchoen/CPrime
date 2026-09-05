#include <future>

template<class Derived, class T> struct Base {
    struct Node { T value; };
    using NodePtr = Node *;
    NodePtr build() { std::future<NodePtr> result; return 0; }
};
template<class T> struct Tree : Base<Tree<T>, T> {
    using Parent = Base<Tree<T>, T>;
    using Node = typename Parent::Node;
    using NodePtr = Node *;
    NodePtr run() { return this->build(); }
};
int main() {
    Tree<int> first;
    Tree<double> second;
    return first.run() != 0 || second.run() != 0;
}

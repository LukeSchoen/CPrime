namespace compatibility
{
class Node;
class Element;

class Node
{
public:
  Element* FirstChildElement();
};

class Element : public Node
{
};

Element sharedElement;

Element* Node::FirstChildElement()
{
  return &sharedElement;
}

class Handle
{
public:
  explicit Handle(Node* initial) : node(initial) {}
  explicit Handle(Node& initial) : node(&initial) {}

  Handle FirstChildElement()
  {
    return Handle(node ? node->FirstChildElement() : 0);
  }

  Node* Get() const
  {
    return node;
  }

private:
  Node* node;
};
}

using namespace compatibility;

int main()
{
  Handle root(sharedElement);
  return root.FirstChildElement().Get() ? 0 : 1;
}

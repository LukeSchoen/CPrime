namespace compatibility
{
class Attribute;

class Element
{
public:
  const Attribute* FindAttribute() const;

  int HasAttribute() const
  {
    const Attribute* attribute = FindAttribute();
    return attribute != 0;
  }
};

class Attribute
{
};

const Attribute* Element::FindAttribute() const
{
  return 0;
}
}

int main()
{
  compatibility::Element element;
  return element.HasAttribute() ? 1 : 0;
}

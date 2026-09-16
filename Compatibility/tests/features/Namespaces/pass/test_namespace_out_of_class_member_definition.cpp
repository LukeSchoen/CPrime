namespace compatibility
{
class Resource
{
public:
  Resource(int value);
  int Value() const;
  char* Text();
  static const char* label;

private:
  int value_;
  char text_[2];
};

Resource::Resource(int value) : value_(value)
{
  text_[0] = 'x';
  text_[1] = 0;
}

int Resource::Value() const
{
  return value_;
}

char* Resource::Text()
{
  return text_;
}

const char* Resource::label = "resource";
}

int main()
{
  compatibility::Resource resource(37);
  return resource.Value() == 37 && resource.Text()[0] == 'x'
         && compatibility::Resource::label[0] == 'r' ? 0 : 1;
}

namespace compatibility
{
class Catalog
{
public:
  enum Count
  {
    NAME_COUNT = 2
  };

  static const char* names[NAME_COUNT];
};

const char* Catalog::names[NAME_COUNT] = { "alpha", "beta" };
}

int main()
{
  return compatibility::Catalog::names[0][0] == 'a'
             && compatibility::Catalog::names[1][0] == 'b'
           ? 0 : 1;
}

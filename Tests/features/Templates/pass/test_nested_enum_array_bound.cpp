#include <array>
struct Palette {
  enum class Index {First,Second,Max};
  typedef std::array<unsigned,(unsigned)Index::Max> Colors;
  Colors values;
};
int main(){Palette p;p.values[1]=9;return p.values.size()!=2||p.values[1]!=9;}

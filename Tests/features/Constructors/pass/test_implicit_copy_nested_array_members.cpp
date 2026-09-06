struct Block {
  float values[16];
  Block() = default;
  static Block make(float start)
  {
    Block result;
    for (int i = 0; i < 16; ++i) result.values[i] = start + i;
    return result;
  }
  Block operator+(const Block& other) const
  {
    Block result;
    for (int i = 0; i < 16; ++i) result.values[i] = values[i] + other.values[i];
    return result;
  }
};

int copied, assigned;
struct Element {
  int value;
  Element() : value(0) {}
  Element(const Element& other) : value(other.value) { ++copied; }
  Element& operator=(const Element& other)
  {
    value = other.value;
    ++assigned;
    return *this;
  }
};
struct Grid { Element cells[2][3]; };

int main()
{
  Block first = Block::make(1);
  Block result = Block(first + Block::make(2));
  for (int i = 0; i < 16; ++i)
    if (result.values[i] != 3 + 2 * i) return 1;
  Grid original;
  for (int row = 0; row < 2; ++row)
    for (int col = 0; col < 3; ++col) original.cells[row][col].value = row * 3 + col;
  Grid copy = Grid(original);
  if (copied != 6 || assigned) return 2;
  Grid replacement;
  replacement = copy;
  if (copied != 6 || assigned != 6) return 3;
  for (int row = 0; row < 2; ++row)
    for (int col = 0; col < 3; ++col)
      if (replacement.cells[row][col].value != row * 3 + col) return 4;
  return 0;
}

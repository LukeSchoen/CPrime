// EXPECT_EXIT: 0
// `object[{...}]` initializes the operator[] parameter from a braced-init-list.
// The overload is selected by argument count and the list is materialized
// against the chosen parameter type, as a chunk grid relies on with
// `(*pChunk)[{ix, iy, iz}]`.
struct Position
{
  int x, y, z;
};

struct Grid
{
  int sums[4];

  int &operator[](const Position &position) { return sums[position.x]; }
  int operator[](const Position &position) const { return position.x + position.y + position.z; }
};

int main()
{
  Grid grid = { { 1, 2, 3, 4 } };
  if (grid[{1, 2, 3}] != 2) return 1;
  grid[{2, 0, 0}] = 9;
  if (grid.sums[2] != 9) return 2;
  const Grid &readOnly = grid;
  if (readOnly[{2, 0, 0}] != 2) return 3;
  return 0;
}

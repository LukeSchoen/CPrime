int main() {
  int values[3] = {2, 5, 7};
  int grid[2][2] = {{3, 4}, {8, 9}};
  auto update = [&values, &grid]() {
    values[1] += grid[1][0];
    grid[0][1] += values[0];
    return values[1] + grid[0][1];
  };
  if (update() != 19 || values[1] != 13 || grid[0][1] != 6) return 1;
  auto implicit = [&]() { return values[2] + grid[1][1]; };
  return implicit() != 16;
}

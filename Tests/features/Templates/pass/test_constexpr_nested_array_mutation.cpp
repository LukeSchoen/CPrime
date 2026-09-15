constexpr int update() {
  int values[2][2] = {{1, 2}, {3, 4}};
  int &alias = values[1][0];
  values[0][0] = 7;
  values[1][1] += 3;
  alias++;
  return values[0][0] + values[1][0] + values[1][1];
}
static_assert(update() == 18);
int main() { return update() != 18; }

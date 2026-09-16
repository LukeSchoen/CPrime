// EXPECT_COMPILE_ARGS: -std=c++17
// Coverage: a structured binding declared in a selection or iteration
// initializer. The bindings decompose a single hidden object and stay visible
// in the header and the body of the statement.
struct Pair {
  int first;
  int second;
};

static Pair make_pair(int first, int second)
{
  Pair pair{first, second};
  return pair;
}

int main()
{
  int seen = 0;
  if (auto [first, second] = make_pair(1, 2); first + second == 3) {
    if (first != 1 || second != 2) return 1;
    seen = first;
  } else {
    return 2;
  }

  for (auto [first, second] = make_pair(3, 4); seen < 3; ++seen) {
    if (first != 3 || second != 4) return 3;
  }

  switch (auto [first, second] = make_pair(5, 6); first + second) {
    case 11: seen += first; break;
    default: return 4;
  }

  return seen != 8;
}

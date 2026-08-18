class Pair
{
public:
  Pair(int px, int py) : x(px), y(py) {}
  int x;
  int y;
};

Pair MakePair()
{
  return { 3, 7 };
}

Pair MakeConditionalPair(int x, int y, int threshold)
{
  return { x > threshold ? x : 0, y > threshold ? y : 0, };
}

int main()
{
  Pair pair = MakePair();
  Pair conditional = MakeConditionalPair(5, 2, 3);
  return pair.x == 3 && pair.y == 7
         && conditional.x == 5 && conditional.y == 0 ? 0 : 1;
}

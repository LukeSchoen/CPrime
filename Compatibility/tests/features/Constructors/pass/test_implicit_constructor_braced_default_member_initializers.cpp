struct Pair
{
  int first;
  int second;
};

struct Box
{
  int value;
  explicit Box(int input) : value(input) {}
};

struct Widget
{
  int scalar = {7};
  int zero = {};
  Pair pair = {1, 2};
  Box box = {9};
};

int main()
{
  Widget widget;
  if (widget.scalar != 7) return 1;
  if (widget.zero != 0) return 2;
  if (widget.pair.first != 1 || widget.pair.second != 2) return 3;
  if (widget.box.value != 9) return 4;
  return 0;
}

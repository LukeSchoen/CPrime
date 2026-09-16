struct NullptrOverloadTarget
{
  int selected;

  NullptrOverloadTarget() : selected(0) {}

  void Set(int *, int) { selected = 1; }
  void Set(float *, float) { selected = 2; }
};

int main()
{
  NullptrOverloadTarget target;
  target.Set(nullptr, 7);
  return target.selected == 1 ? 0 : 1;
}

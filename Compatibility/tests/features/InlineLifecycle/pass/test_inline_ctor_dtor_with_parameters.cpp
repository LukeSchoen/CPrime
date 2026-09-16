// EXPECT_EXIT: 0
#include <stdlib.h>

struct Widget
{
  int value;

  Widget(int seed)
  {
    this->value = seed;
  }

  ~Widget(int *out)
  {
    *out += this->value;
  }
};

void Widget_constructor(struct Widget *this, int seed);
void Widget_destructor(struct Widget *this, int *out);

int main(void)
{
  int total = 0;
  struct Widget *w = (struct Widget *)malloc(sizeof(struct Widget));
  if (!w)
    return 1;

  Widget_constructor(w, 7);
  Widget_destructor(w, &total);
  free(w);

  return total == 7 ? 0 : 2;
}
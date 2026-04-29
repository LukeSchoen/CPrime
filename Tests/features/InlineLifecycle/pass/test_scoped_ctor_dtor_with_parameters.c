// EXPECT_EXIT: 0
#include <stdlib.h>

struct Tracker
{
  int value;

  Tracker(int seed);
  ~Tracker(int *out);
};

Tracker::Tracker(int seed)
{
  this->value = seed;
}

Tracker::~Tracker(int *out)
{
  *out += this->value;
}

void Tracker_constructor(struct Tracker *this, int seed);
void Tracker_destructor(struct Tracker *this, int *out);

int main(void)
{
  int total = 0;
  struct Tracker *t = (struct Tracker *)malloc(sizeof(struct Tracker));
  if (!t)
    return 1;

  Tracker_constructor(t, 11);
  Tracker_destructor(t, &total);
  free(t);

  return total == 11 ? 0 : 2;
}
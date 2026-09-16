// EXPECT_STDOUT: global array lifetime ok
#include <stdio.h>

static int constructed;
static int destroyed;
static int nextDestruction;
static int incorrectOrder;

struct Item {
  const Item *self;
  const char *text;
  int identity;
  Item(const char *value = "default")
      : self(this), text(value), identity(++constructed) {}
  Item(const Item &other)
      : self(this), text(other.text), identity(++constructed) {}
  ~Item() {
    if (self != this || identity != nextDestruction--) incorrectOrder = 1;
    ++destroyed;
  }
};

struct FinalCheck {
  FinalCheck() {}
  ~FinalCheck() {
    if (!incorrectOrder && constructed == 19 && destroyed == 19)
      puts("global array lifetime ok");
  }
};

static FinalCheck check;
static const Item single = "single";
static const Item names[] = {"first", "second"};
static Item tail[4] = {"third"};
static Item grid[2][2] = {{"fourth", "fifth"}, {"sixth"}};
static Item defaults[2];
static const Item copies[] = {single, names[0]};
static const Item flat[][2] = {"seventh", "eighth", "ninth"};

int main() {
  nextDestruction = constructed;
  if (constructed != 19 || destroyed != 0) return 1;
  if (sizeof(names) != 2 * sizeof(Item) || sizeof(copies) != 2 * sizeof(Item)) return 2;
  if (single.text[0] != 's' || names[0].text[0] != 'f' || names[1].text[0] != 's') return 3;
  if (tail[0].text[0] != 't' || tail[3].text[0] != 'd') return 4;
  if (grid[0][1].text[0] != 'f' || grid[1][0].text[0] != 's'
      || grid[1][1].text[0] != 'd') return 5;
  if (defaults[0].self != &defaults[0] || defaults[1].self != &defaults[1]) return 6;
  if (copies[0].text != single.text || copies[1].text != names[0].text) return 7;
  if (sizeof(flat) != 4 * sizeof(Item) || flat[1][0].text[0] != 'n'
      || flat[1][1].text[0] != 'd') return 8;
  return 0;
}

/* A '(' whose next token cannot begin a type specifier is grouping in C, so
   the tentative cast probe is skipped there.  Casts still have to work: a
   typedef name, a type keyword, an abstract declarator, a compound literal
   and a statement expression all keep the type interpretation. */

typedef unsigned long long u64;

struct point {
  int x, y;
};

enum flags {
  flag_a = 3,
  flag_b = 5
};

static struct point make_point(int x, int y)
{
  struct point result = { x, y };
  return result;
}

/* The macro-shaped scalar constants that make the skip worth having. */
static int opcode_table[] = { ((8) | ((0) << 13)),
                              ((4) | ((2) << 13)),
                              (flag_a | (flag_b << 3)) };
static int folded = (1 + 2) * ((3) - 1);
static int negated = -((4) | ((1) << 13));
static int nested = ((((1) + (2))) * (3));
static u64 masked = ((u64) (((3841) & 65280) >> 8));
static unsigned long shifted = (unsigned long) (1 << 20);
static char *text = ("literal");
static int sizes = sizeof(int) + sizeof((char) 1);
static int *address = (&folded);

int main(void)
{
  int value = 6;
  struct point origin = make_point(1, 2);
  int compound = (int) {7};
  int statement = ({
    int inner = 4;
    inner + 1;
  });
  int grouped = (value) + ((2) * 3);
  int widened = (int) (long) value;
  int through_typedef = (int) (u64) value;

  if (opcode_table[0] != (8 | (0 << 13))) return 1;
  if (opcode_table[1] != (4 | (2 << 13))) return 2;
  if (opcode_table[2] != (3 | (5 << 3))) return 3;
  if (folded != 6) return 4;
  if (negated != -((4) | (1 << 13))) return 5;
  if (nested != 9) return 6;
  if (masked != ((3841 & 65280) >> 8)) return 7;
  if (shifted != (1UL << 20)) return 8;
  if (text[0] != 'l' || text[6] != 'l') return 9;
  if (sizes != (int) (sizeof(int) + sizeof(char))) return 10;
  if (*address != 6) return 11;
  if (compound != 7) return 12;
  if (statement != 5) return 13;
  if (grouped != 12) return 14;
  if (widened != 6) return 15;
  if (through_typedef != 6) return 16;
  if (origin.x != 1 || origin.y != 2) return 17;
  if ((u64) ((1) | ((2) << 4)) != (1 | (2 << 4))) return 18;
  if ((long) ((value) * (value)) != 36) return 19;
  return 0;
}

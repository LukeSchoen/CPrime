// EXPECT_EXIT: 0
/* A conditional left operand stores into the arm the condition selects: a
   bit-field arm has no address and the arms may describe different bit
   layouts.  The right operand of a compound assignment is evaluated before
   the left operand (C++17 [expr.ass]). */
struct Low { int plain; int field : 7; };
struct High { int plain; int high : 7; };

static int counter;
static int seen;

int step () { ++counter; return 2; }
int addend () { ++counter; return 40; }
int observe () { seen = counter; ++counter; return 1; }

Low low;
High high;

int main () {
  bool flag = false;

  low.field = 5;
  counter = 0;
  (flag ? low.plain : low.field) = step ();
  if (low.plain != 0 || low.field != 2 || counter != 1) return 1;

  low.plain = 0;
  low.field = 5;
  counter = 0;
  (true ? low.plain : low.field) = step ();
  if (low.plain != 2 || low.field != 5 || counter != 1) return 2;

  low.field = 5;
  high.high = 5;
  counter = 0;
  (flag ? low.field : high.high) += addend ();
  if (low.field != 5 || high.high != 45 || counter != 1) return 3;

  low.plain = 0;
  low.field = 0;
  counter = 0;
  seen = -1;
  (observe () ? low.plain : low.field) += addend ();
  if (seen != 1 || counter != 2 || low.plain != 40 || low.field != 0)
    return 4;

  low.plain = 0;
  low.field = 0;
  counter = 0;
  if (((false ? low.plain : low.field) = step ()) != 2) return 5;
  if (low.field != 2 || low.plain != 0 || counter != 1) return 6;
  counter = 0;
  (low.field = step()) += addend();
  return low.field != 42 || counter != 2;
}

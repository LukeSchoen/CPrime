// EXPECT_EXIT: 0
// A return that builds its result object in the caller's storage owns a
// cleanup for that object while the return statement runs.  The cleanup is not
// part of the ordinary path: the caller takes the object as soon as the return
// completes, so a later return statement in the same function must not replay
// it.  Replaying it destroyed the caller's result object through a second
// return statement, and a class with an owning pointer died on the resulting
// double release.
int live;
int destroyed;

struct Value
{
  int id;

  Value() : id(0) { ++live; }
  Value(int value) : id(value) { ++live; }
  ~Value()
  {
    --live;
    ++destroyed;
  }
};

Value pick(int which)
{
  if (which == 0)
    return Value();
  Value named(which);
  return named;
}

Value pair(int which)
{
  if (which == 0)
    return Value();
  return Value(which);
}

int main()
{
  {
    Value first = pick(0);
    if (first.id != 0) return 1;
    if (destroyed != 0) return 2;
    if (live != 1) return 3;
  }
  if (destroyed != 1) return 4;
  if (live != 0) return 5;

  destroyed = 0;
  {
    Value second = pick(7);
    if (second.id != 7) return 6;
    if (destroyed != 0) return 7;
    if (live != 1) return 8;
  }
  if (destroyed != 1) return 9;

  destroyed = 0;
  {
    Value third = pair(0);
    if (third.id != 0) return 10;
    if (destroyed != 0) return 11;
  }
  if (destroyed != 1) return 12;

  destroyed = 0;
  {
    Value fourth = pair(9);
    if (fourth.id != 9) return 13;
    if (destroyed != 0) return 14;
  }
  return destroyed == 1 ? 0 : 15;
}

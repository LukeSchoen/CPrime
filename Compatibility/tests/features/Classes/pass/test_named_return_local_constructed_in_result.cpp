// EXPECT_EXIT: 0
// A local returned by name is built in the caller's result object, so a
// member that records the object's own address still names the result, and
// the callee must not destroy what the caller now owns.
static int alive, destroyed;

struct Tracked
{
  const Tracked *self;
  int value;

  Tracked(int number) : self(this), value(number) { ++alive; }
  ~Tracked() { --alive; ++destroyed; }
};

struct Owner
{
  int value;
  Owner(int number) : value(number) { ++alive; }
  ~Owner() { --alive; ++destroyed; }
};

Tracked makeTracked(int number)
{
  Tracked local(number);
  return local;
}

Owner makeOwner()
{
  Owner first(1);
  Owner result(2);
  return result;
}

int main()
{
  {
    Tracked returned = makeTracked(7);
    if (returned.self != &returned || returned.value != 7)
      return 1;
    if (alive != 1 || destroyed != 0)
      return 2;
  }
  if (alive != 0 || destroyed != 1)
    return 3;
  {
    // The object returned by name is the caller's; the local declared before
    // it is still destroyed when the callee returns.
    Owner returned = makeOwner();
    if (returned.value != 2)
      return 4;
    if (alive != 1 || destroyed != 2)
      return 5;
  }
  return alive == 0 && destroyed == 3 ? 0 : 6;
}

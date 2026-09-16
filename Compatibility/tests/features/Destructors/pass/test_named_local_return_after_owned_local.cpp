// EXPECT_EXIT: 0
// Returning a named local elides the result copy and hands the caller the
// object.  A class local with a destructor declared before the returned
// object used to leave the frame's cleanup chain cyclic, so compilation of
// the function never terminated.
int destroyed;

struct Owner
{
  int value;
  Owner(int v) : value(v) {}
  ~Owner() { ++destroyed; }
};

Owner make()
{
  Owner first(1);
  Owner result(2);
  return result;
}

int main()
{
  {
    Owner copy = make();
    if (copy.value != 2)
      return 1;
    if (destroyed != 1)
      return 2;
  }
  return destroyed == 2 ? 0 : 3;
}

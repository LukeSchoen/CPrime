// EXPECT_EXIT: 0
// A nested anonymous union injects its members into the enclosing anonymous
// union, so the whole chain reaches the surrounding scope.  The inner union
// used to be mistaken for a named nested class declaration, which dropped the
// member entirely: `d` was undeclared at block scope and unreachable through
// an object.

template <typename T>
void nested_block_scope ()
{
  union {
    union { T d; };
  };
  d = T (7);
  if (d != T (7))
    d = T (0);
}

struct Holder
{
  struct {
    union { int value; };
  };
};

int main ()
{
  union {
    union { int d; };
  };
  d = 42;
  if (d != 42)
    return 1;

  Holder h;
  h.value = 9;
  if (h.value != 9)
    return 2;

  nested_block_scope<int> ();
  return 0;
}

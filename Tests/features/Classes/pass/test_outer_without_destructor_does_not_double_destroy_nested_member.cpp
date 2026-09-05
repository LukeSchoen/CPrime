struct NestedCleanupLeaf
{
  static int destructorCalls;

  ~NestedCleanupLeaf()
  {
    ++destructorCalls;
  }
};

int NestedCleanupLeaf::destructorCalls = 0;

struct NestedCleanupOwner
{
  NestedCleanupLeaf leaf;

  ~NestedCleanupOwner()
  {
    // The owner body and its implicit member destruction each run once.
    // The enclosing aggregate must not independently destroy this leaf again.
    ++NestedCleanupLeaf::destructorCalls;
  }
};

struct NestedCleanupOuter
{
  NestedCleanupOwner owner;
};

int main()
{
  {
    NestedCleanupOuter value;
  }
  return NestedCleanupLeaf::destructorCalls == 2 ? 0 : 1;
}

// EXPECT_EXIT: 0

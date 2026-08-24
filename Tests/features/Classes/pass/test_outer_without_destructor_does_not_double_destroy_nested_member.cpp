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
    // Model an owning field destructor: once the owner destructor is selected,
    // the outer aggregate must not also register the nested leaf separately.
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
  return NestedCleanupLeaf::destructorCalls == 1 ? 0 : 1;
}

// EXPECT_EXIT: 0

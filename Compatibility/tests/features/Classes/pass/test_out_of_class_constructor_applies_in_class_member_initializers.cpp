struct DefaultMemberChild
{
  int value;

  DefaultMemberChild()
    : value(7)
  {
  }
};

struct DefaultMemberOwner
{
  DefaultMemberChild child;
  bool enabled = false;
  int resourceId = -1;

  DefaultMemberOwner(int id);
};

DefaultMemberOwner::DefaultMemberOwner(int id)
  : resourceId(id)
{
}

int main()
{
  DefaultMemberOwner owner(23);
  return owner.child.value == 7
         && !owner.enabled
         && owner.resourceId == 23 ? 0 : 1;
}

// EXPECT_EXIT: 0

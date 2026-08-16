struct FriendTarget;

int WriteFriendTarget(const FriendTarget *values, int count);

struct FriendTarget
{
  int value;
  friend int WriteFriendTarget(const FriendTarget *values, int count);
};

int WriteFriendTarget(const FriendTarget *values, int count)
{
  return count ? values[0].value : 0;
}

int main()
{
  FriendTarget target;
  target.value = 7;
  return WriteFriendTarget(&target, 1) == 7 ? 0 : 1;
}

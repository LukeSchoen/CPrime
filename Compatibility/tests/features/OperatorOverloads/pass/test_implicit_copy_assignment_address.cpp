// EXPECT_EXIT: 0

struct Member
{
  int value;
  ~Member() {}
};

struct Owner
{
  Member member;
  int tag;
};

int main()
{
  Owner source = Owner();
  Owner target = Owner();
  Owner copied(source);
  Owner assigned = Owner();
  Owner &(Owner::*assign)(const Owner &) = &Owner::operator=;

  source.member.value = 7;
  source.tag = 11;
  (target.*assign)(source);

  if (target.member.value != 7)
    return 1;
  if (target.tag != 11)
    return 2;
  if (copied.member.value != 0 || copied.tag != 0)
    return 3;
  if (assigned.member.value != 0 || assigned.tag != 0)
    return 4;
  return 0;
}

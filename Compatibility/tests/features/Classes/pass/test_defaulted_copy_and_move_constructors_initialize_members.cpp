struct Member
{
  int value;

  Member() : value(0) {}
  Member(const Member &other) : value(other.value) {}
  Member(Member &&other) : value(other.value)
  {
    other.value = 0;
  }
};

struct Owner
{
  Member member;

  Owner() = default;
  Owner(const Owner &) = default;
  Owner(Owner &&) = default;
};

int main()
{
  Owner source;
  source.member.value = 31;

  Owner copied(source);
  if (copied.member.value != 31 || source.member.value != 31)
    return 1;

  Owner moved(static_cast<Owner &&>(source));
  return moved.member.value == 31 && source.member.value == 0 ? 0 : 2;
}

// EXPECT_EXIT: 0
